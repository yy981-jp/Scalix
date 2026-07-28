# MediaPipe Pose → Avatar Pose Solver 設計方針

## 目的

MediaPipe Poseの33個のLandmarkから、アバターの各ボーンのローカル回転を計算する仕組みを実装する。

Landmark座標をそのままボーンへ適用するのではなく、**姿勢（回転）を推定して適用する**ことを目的とする。

---

# 基本方針

MediaPipeが返すものは

* position (x,y,z)
* visibility

のみであり、QuaternionやEuler角は含まれない。

そのため

```
Landmark
	↓
骨方向ベクトル
	↓
姿勢推定
	↓
Quaternion
	↓
Node.trs.rot
```

という流れで処理する。

位置を直接Nodeへ設定しない。

---

# 設計

MediaPipeの生データは直接扱わず、一度Pose専用クラスへ変換する。

例

```cpp
struct PoseLandmark {
	vec3f pos;
	float visibility;
};

struct PoseFrame {
	std::array<PoseLandmark, 33> landmarks;
};
```

---

# 人体構造

Landmarkは単なる配列ではなく、人体スケルトンとして扱う。

最低限、

* parent
* children
* bone connection

を保持する。

例

```cpp
struct LandmarkNode {
	LandmarkId id;
	LandmarkId parent;
	std::vector<LandmarkId> children;
};
```

もしくは

```cpp
struct BoneConnection {
	LandmarkId parent;
	LandmarkId child;
};
```

---

# Bone情報

各Boneについて

* 親Landmark
* 子Landmark
* BindPoseでの方向
* BindPoseでの長さ

を保持する。

例

```cpp
struct PoseBone {
	LandmarkId parent;
	LandmarkId child;

	float restLength;
	vec3f restDirection;
};
```

---

# 骨方向

各Boneについて

```cpp
direction =
normalize(child.pos - parent.pos);
```

を計算する。

長さではなく方向を使用する。

---

# Quaternion生成

BindPose方向

```
restDirection
```

現在方向

```
currentDirection
```

から

```
rotation(restDirection, currentDirection)
```

を計算する。

Nodeへ設定するのはPositionではなくRotationのみ。

---

# 平面法線

方向ベクトルだけではTwistが求まらない。

3点から平面法線を計算する。

例

```
Shoulder
↓

Elbow
↓

Wrist
```

```cpp
upper =
normalize(elbow - shoulder);

lower =
normalize(wrist - elbow);

normal =
normalize(cross(upper, lower));
```

そこから

```
Forward
Right
Up
```

の3軸を構築し、Rotation MatrixまたはQuaternionを生成する。

---

# visibility

visibilityは真偽値ではなく信頼度として扱う。

禁止事項

```
visibility < threshold
→ Landmarkを無視する
```

のような実装。

推奨

```
visibilityが低い
	↓
前フレーム
	↓
周囲Landmark
	↓
骨長
```

を利用して推定する。

---

# Visibility補完

例

```
A
|
B
|
C
```

Bのみvisibilityが低い場合

優先順位

1. 前フレーム
2. 骨長維持
3. A,Cから補間

単純補間だけで済ませない。

---

# 骨長

人体の骨長は一定である。

初期フレームまたはAvatarのBindPoseから

```
restLength
```

を保持する。

Visibility低下時やノイズ補正時は骨長制約を利用する。

---

# 平滑化

Landmarkは毎フレーム微妙に揺れる。

最低限

* EMA
* 前フレーム補間

などを導入できる構造にする。

---

# Solver

MediaPipeとAvatarの間にSolver層を配置する。

```
MediaPipe
	↓
PoseFrame
	↓
PoseSolver
	・visibility補正
	・平滑化
	・骨長補正
	・方向ベクトル生成
	・Quaternion生成
	↓
Avatar(Node.trs.rot)
```

MediaPipe依存コードをAvatar側へ持ち込まない。

---

# SpringBoneとの共通点

SpringBoneChainで保持している

* restLength
* restDirection

という考え方はPoseSolverでも活用できる。

ただしSpringBoneは物理シミュレーションであり、PoseSolverは姿勢推定であるため責務は分離する。

コードの共通化は可能だが、クラスは分ける。

---

# 実装方針

実装は以下の順番で進める。

1. Landmark管理クラス
2. 人体スケルトン構造
3. Bone情報生成
4. Direction生成
5. Quaternion生成
6. Avatarへ適用
7. visibility補正
8. 平滑化
9. Twist補正
10. IKやConstraintの追加

---

# コーディング方針

* C++23
* K&Rスタイル
* インデントはTab
* 役割ごとに責務を分離する
* 可読性を優先する
* ハードコードされたLandmark番号を避け、enum class LandmarkIdを使用する
* 将来的にMediaPipe以外（OpenXR、VRトラッカー、BVH等）も入力ソースとして利用できる設計を目指す
