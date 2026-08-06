# DebugDraw実装依頼

Scalixエンジンに、開発用のシンプルなDebugDraw機能を追加してください。

## 目的

MediaPipeやIK、SpringBoneなどのデバッグで、3D空間内に簡単な図形を描画できるようにしたいです。

本格的なGizmoではなく、軽量なデバッグ用途を想定しています。

## 要件

まずは以下の機能だけ実装してください。

* `drawLine(vec3f a, vec3f b)`
* `drawCross(vec3f position, float size)`

`drawCross()` はX/Y/Z方向に短い線を描画するだけで構いません。

例:

```
      |
------ + ------
      |
     /
```

実装は内部で `drawLine()` を利用してください。

---

## 実装方針

* bgfxを使用すること
* 毎フレーム呼び出して使えること
* デバッグ専用なので多少非効率でも構わない
* アバターやMeshシステムには依存しないこと
* glTF等のアセットは不要
* 専用のDebugDrawクラスまたはDebugRendererクラスとして実装すること

---

## API例

```cpp
debugDraw.drawCross({0,0,0}, 0.05f);

debugDraw.drawLine(
    {0,0,0},
    {1,0,0}
);
```

---

## 将来的な拡張

後から以下を追加できる設計にしてください。

* drawCube()
* drawSphere()
* drawAxes()
* drawBone()
* drawFrustum()

今回は実装不要ですが、APIを追加しやすい構成にしてください。

---

## 重要

既存のRendererやMeshシステムへの影響は最小限にしてください。

デバッグ表示専用の独立した機能として実装してください。

---

## その他
DebugDrawはengine/src/debug以下に作り、どこからでも呼び出せるglobal-instanceであること