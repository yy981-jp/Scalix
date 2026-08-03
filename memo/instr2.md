# PoseSolver 調査引継ぎメモ

## 1. 調査目的

- `tracker/replay.py` で送信した `A` ポーズが engine 側でどのように処理されているかを追跡する。
- 特に `engine/src/tracker/poseSolver.cpp` のキャリブレーションとボーン回転適用の挙動を解明する。
- 問題が `PoseSolver` 内の計算ロジック（キャリブレーション基準、ランドマーク→ボーン対応、回転適用）にあるかを確認する。

## 2. 現状と確認済み事項

### 2.1 再現経路

- `tracker/replay.py` が `records/a.bin` を読み込み、UDP 51801 へ 30Hz で送信する。
- `engine/src/tracker/recv.cpp` で protobuf `PoseFrame` を受信し、MediaPipe 座標系から `scalix` 座標系へ変換して `PoseFrame` に詰める。
- `engine/src/core/engine.cpp` の `Game::update()` で `receiver.tick(frame)` が true を返した場合、`poseSolver->solve(frame)` を呼ぶ。

### 2.2 `PoseSolver` の設計

- `PoseSolver` は `Avatar` の humanoid ボーンを `PoseBone` として構成する。
- 各 `PoseBone` は以下を保持する。
  - `humanoidBone`
  - `landmarks`（親子 Landmark）
  - `node` / `childNode`
  - `restLength`
  - `restDirection`
  - `restRotation`
  - `calibratedDirection`
- 初回キャリブレーション前は `calibratedDirection = restDirection` でフォールバックする。

### 2.3 キャリブレーション/solve の現在の挙動

- `PoseSolver::solve()` はまず `smoothAndRepair(frame)` を実行し、ランドマークのスムージングと骨長補正を行う。
- `needsCalibration` が true の場合、`calibrate(frame)` を呼び出す。
- `calibrate()` は各ボーンについて親子 Landmark の方向を求め、`avatar.trackingTransforms` に基づく親空間へ変換した向きで `calibratedDirection` を更新する。
- `solve()` は各ボーンの現在方向を同じ親空間へ変換し、 `quatFromTo(calibratedDirection, currentDirection)` を算出して `node.trs.rot` に適用する。

## 3. 追加したデバッグ内容

- `engine/src/tracker/poseSolver.cpp` に以下のログ出力を追加済み
  - `PoseSolver calibrate skip bone=...`：可視性不足や方向ベクトル無しでスキップした骨
  - `PoseSolver calibrate bone=... calDir=... restDir=...`：キャリブレーション済みの向き
  - `PoseSolver calibrate succeeded`
  - `PoseSolver::solve needsCalibration=... bones=...`
  - `PoseSolver bone=... curDir=... calDir=... restDir=... quat=...`
- これにより、キャリブレーション基準と現在方向の違い、Quaternion 計算結果、ボーン適用前後の状態を追えるようにした。

## 4. 実際に確認した `A` ポーズのランドマーク値

- `records/a.bin` の主要ランドマークは 33 個。特に以下の landmark の値を確認済み。
  - LeftShoulder, RightShoulder
  - LeftElbow, RightElbow
  - LeftWrist, RightWrist
  - LeftHip, RightHip
  - LeftKnee, RightKnee
  - LeftAnkle, RightAnkle
- `RightHip` / `RightKnee` / `RightAnkle` あたりは visibility が低く、キャリブレーションや solve の対象にならない可能性が高い。

## 5. 課題と注力ポイント

### 5.1 キャリブレーション基準のズレ

- `PoseSolver` はモデルの rest 方向ではなく、初回キャリブレーション時の観測方向を基準にしている。
- そのため、最初に受信した `A` ポーズがキャリブレーション姿勢として使われると、以降の回転計算はその姿勢からの相対回転になる。
- ここで期待値と異なる場合は、`restDirection` / `calibratedDirection` の変換座標系やボーン対応にズレがある可能性がある。

### 5.2 ボーンの親空間変換

- `directionInParentSpace()` は `avatar.trackingTransforms` の親ノード逆行列を使ってベクトルを変換する。
- ここが正しくないと、キャリブレーションも solve も誤った軸で回転を算出する。

### 5.3 `node.trs.rot` 適用後の最終描画

- `AvatarSystem::update()` で `avatar.globalTransforms` と `avatar.trackingTransforms` を再計算している。
- `PoseSolver` が `node.trs.rot` を更新しても、`AvatarSystem` の更新順序で正しく反映されるか確認が必要。

## 6. 次の調査手順

1. `PoseSolver` のログを実際に `sl.exe` で出力し、キャリブレーション成功/失敗と quaternion の値を確認する。
2. `A` ポーズ送信中に `needsCalibration` が `false` になるか、どのタイミングで `calibrate()` が完了するか確認する。
3. `restDirection` と `calibratedDirection` の差が大きい骨を重点的に調べる。
4. `directionInParentSpace()` の親行列変換が正しいか、`trackingTransforms` の使い方に問題がないか確認する。
5. `PoseSolver::solve()` 直後に `node.trs.rot` が想定どおりにセットされているか、`AvatarSystem::update()` の順序で破棄されていないかチェックする。

## 7. 参照ファイル

- `tracker/replay.py`
- `engine/src/tracker/pose.h`
- `engine/src/tracker/recv.cpp`
- `engine/src/tracker/poseSolver.cpp`
- `engine/src/core/engine.cpp`
- `engine/src/core/avatar.cpp`
- `engine/src/core/avatarSystem.cpp`

## 8. 注意点

- `replay.py` は UDP 送信のみで、送信先は `127.0.0.1:51801`。
- `PoseSolver` のデバッグ出力は `stdout` へ表示されるため、`sl.exe` を起動したターミナルで確認する。
- `A` ポーズの `RightHip` 側は visibility が低いので、下半身の solve 結果が不安定になりやすい。
