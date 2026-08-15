# Scalix 3D描画・MotionCapture問題 引き継ぎ資料

## 0. プロジェクト概要

C++23 / MSYS2 MinGW64 / bgfx / Qt6 で開発している3Dアバターエンジン「Scalix」。

現在、MotionCapture部分はかなり完成してきたが、**描画時のアバターの回転が破綻している**。

症状：

* アバターをyaw回転させると、頭・体上部・体下部・足などが互いに逆方向へ開くような挙動をする
* MotionCaptureそのものの骨回転計算は改善してきている
* 現在は描画システム / glTF / skinning / Transformあたりを疑って調査中
* 過去に応急処置的な座標変換・符号反転・行列順変更などを重ねており、設計が一貫していない可能性が高い

---

# 1. まず分かったこと：root合成順は今回の直接原因ではない

現在のコードでは概念的に、

```cpp
out[idx] = node.trs * entityTransform;
```

となっている。

これはgit履歴 `7daa3df1`（2026-07-15）で、

```cpp
- avatar.globalTransforms[idx] = entityTransform * node.trs;
+ avatar.globalTransforms[idx] = node.trs * entityTransform;
```

に変更された。

理由は `bx::mtxMul` の従来の積順に合わせるため。

つまり設計上の明確な数学的根拠より、**bxのAPI都合で暫定的に変更されたもの**。

ただし、今回の実モデルのscene rootは完全identity：

```text
node[291] Shinano_AMS
translation = (0, 0, 0)
rotation = identity
scale = (1, 1, 1)
negative scale = none
```

したがって、

```text
rootLocal * entity
entity * rootLocal
```

は同じ。

よって**今回のモデルについてroot合成順は直接原因ではない**。

---

# 2. 実モデルの重要なnode hierarchy

```text
Shinano_AMS [291]
└─ Armature [290]
   └─ Body [278]
```

scene root `Shinano_AMS` はidentity。

一方、`Armature [290]` に、

```text
rotation = (-0.7071069, 0, 0, 0.7071066)
```

という約 -90° のX回転が入っている。

つまりモデル内部に座標系補正と思われるrotationが存在する。

これはloader側で勝手に付与されたものではなく、**モデルのnode hierarchy内に保存されている回転**。

---

# 3. yawを変えたときのCPU hierarchy監査

yaw = 0 / 90 / 180 / 270°について、head / chest / hips / legなどのglobal transformを調査済み。

全jointが同じworld Y yawを受けている。

例：

```text
yaw 0:
head  = (0, 1.13233, -0.01236)
chest = (0, 0.19479,  0.98085)
hips  = (0, 0,        1)
leg   = (0.12387, 0.00214, -0.99230)

yaw 90:
head  = (-0.01236, 1.13233, 0)
chest = (0.98085, 0.19479, 0)
hips  = (1, 0, 0)
leg   = (-0.99230, 0.00214, -0.12387)

yaw 180:
head  = (0, 1.13233, 0.01236)
chest = (0, 0.19479, -0.98085)
hips  = (0, 0, -1)
leg   = (-0.12387, 0.00214, 0.99230)

yaw 270:
head  = (0.01236, 1.13233, 0)
chest = (-0.98085, 0.19479, 0)
hips  = (-1, 0, 0)
leg   = (0.99230, 0.00214, 0.12387)
```

したがって、

> CPU側のglTF node hierarchyだけを見ると「一部jointだけ逆yaw」という現象は発生していない。

---

# 4. Transform / Mtxについて

`Transform` はTRSと`mtx`を両方保持している。

現在の未コミット変更では、親子合成後に、

```cpp
out.rebuildMatrix();
```

している。

そのため、

```text
global.pos
global.mtx translation
```

は通常のTRS範囲では一致する。

以前は直接行列積を使っており、

```text
global.pos
global.mtx.pos()
```

が異なる状態を実測したことがある。

現在はこの問題を解消する方向。

ただし、

* 非一様scale
* negative scale
* shear

などを含む場合、TRSに再構築するとshearを表現できない。

今回のShinanoモデルではnegative scaleは確認されていないので、今回の主原因とは考えにくい。

---

# 5. glTF `node.matrix` の問題

loaderは現在TRSを読むが、`node.matrix` を処理していない。

つまりmatrix形式のnodeが存在するglTFではlocal transformが失われる可能性がある。

これは**汎用glTF loaderとしては高優先度の問題**。

ただし今回調査しているShinanoのscene rootでは、

```text
node.matrix = unused
```

だった。

今回の直接原因とはまだ断定できないが、後でloaderを整理するときに修正対象。

---

# 6. SkinningのCPU側

現在のpalette生成は、

```cpp
jointMtx[i] =
	avatar.globalTransforms[nodeIdx].mtx *
	skin.invBind[i];
```

という形。

標準的な、

```text
currentJointGlobal * inverseBind
```

の構造。

shader側は、

```hlsl
mul(skin, float4(a_position, 1.0))
```

。

---

# 7. Mtx / bx / HLSLの行列規約

ここはかなり調査済み。

CPUの `Mtx` / bx はrow-vector系の扱い。

`bx::mul(v, m)` は概念的に、

```text
x' = x*m[0] + y*m[4] + z*m[8] + m[12]
```

のようにtranslationが `m[12..14]` にある。

一方HLSL shaderは、

```hlsl
mul(matrix, vector)
```

。

同じ16 floatをCPU側ではrow-vector matrix `R`、HLSL側ではそのtranspose `R^T` として読むことで、

```text
v_r R == R^T v_c
```

となる。

したがって、

**bgfx::setUniform() の前に明示的transposeを入れる必要はない。**

実際、RenderDocでD3D11 shaderを確認した結果もこの方式と整合している。

---

# 8. `Mtx::operator*` の特殊性

`Mtx::operator*` は、

```cpp
bx::mtxMul(res.data(), other.data(), data());
```

のように引数を反転している。

そのため、CPU上の見た目の

```cpp
C = A * B;
```

は、raw row matrixとして見ると、

```text
R_C = R_B * R_A
```

になる。

したがって、**ソースコード上の `A * B` と、数学的なrow-vector matrixの適用順を混同しないこと**。

この部分は今後Transform体系を整理する際に最重要。

---

# 9. RenderDocで実際のVertex Shaderを確認済み

実際に使われているVertex Shader：

```text
Shader hash:
769e96a8-4488e706-82d9aa73-87471409

vs_5_0
```

Vertex Input：

```text
Reg  Semantic       Type
0    BLENDINDICES   uint4
1    POSITION       float3
2    TEXCOORD       float2
3    BLENDWEIGHT    float4
```

したがって、

```text
v0 = BLENDINDICES
v1 = POSITION
v2 = TEXCOORD
v3 = BLENDWEIGHT
```

で確定。

---

# 10. RenderDocで選択した頂点

```text
VTX = 12400
IDX = 2252

POSITION:
(0.00202, 0.98007, 0.0728)

BLENDINDICES:
(6, 15, 0, 0)

BLENDWEIGHT:
(0.99665, 0.00335, 0, 0)

TEXCOORD:
(0.5015, 0.17442)
```

yaw=0°とyaw=90°の両方で**全く同じ頂点属性**だった。

これは当然で、yawはentity transformであってmesh vertex buffer自体を変更しないため。

---

# 11. Shaderのskinning処理

disassemblyから、bone indexは、

```text
index * 4
```

されてconstant bufferへアクセスされる。

bone 6：

```text
6 * 4 + 4 = 28
```

なので、

```text
cb0[28..31]
```

がbone 6のmatrix。

bone 15：

```text
15 * 4 + 4 = 64
```

なので、

```text
cb0[64..67]
```

がbone 15のmatrix。

shaderは、

```text
0.99665 * bone[6]
+
0.00335 * bone[15]
```

を作っている。

その後、

```text
skin * POSITION
```

を計算。

さらに `cb0[0..3]` を使ってMVP変換し、`SV_Position`を出している。

---

# 12. yaw=0° RenderDoc実測

bone 6：

```text
cb0[28] = ( 1.00,     0.00236, -0.00023, 0.00)
cb0[29] = (-0.00236,  1.00,      0.00001, 0.00)
cb0[30] = ( 0.00023, -0.00001,   1.00,    0.00)
cb0[31] = ( 0.00215, -1.58704,  -0.03283, 1.00)
```

bone 15：

```text
cb0[64] = ( 0.98622, -0.16541,  0.00099, 0.00)
cb0[65] = ( 0.16541,  0.98608, -0.01671, 0.00)
cb0[66] = ( 0.00178,  0.01664,  0.99986, 0.00)
cb0[67] = (-0.15475, -1.55362, -0.28293, 1.00)
```

shader最終出力：

```text
r0 = (0.00186, -2.26362, 4.93239, 4.96091)
r1 = (0.5015, 0.17442, -0.0391, -0.03909)
r2 = (0, 0, -0.0391, -0.03909)
r3 = (0, 0, 0, 1)
```

ここで注意：

`r0` はshaderの最後の状態なので**SV_Position**。

---

# 13. yaw=90° RenderDoc実測

同じVTX 12400。

bone 6：

```text
cb0[28] = (-0.25486,  0.18836, -0.94845, 0.00)
cb0[29] = ( 0.18836,  0.97173,  0.14236, 0.00)
cb0[30] = ( 0.94845, -0.14236, -0.28314, 0.00)
cb0[31] = (-0.20399,  0.02824, -0.12996, 1.00)
```

bone 15：

```text
cb0[64] = (-0.13454,  0.95659, -0.25851, 0.00)
cb0[65] = ( 0.98118,  0.16508,  0.10019, 0.00)
cb0[66] = ( 0.13852, -0.24017, -0.96080, 0.00)
cb0[67] = (-1.09299,  0.79352,  0.04800, 1.00)
```

shader最終出力：

```text
r0 = (0.04733, 0.46853, 4.98416, 5.01266)
r1 = (0.5015, 0.17442, 0.01267, 0.01266)
r2 = (0, 0, 0.01267, 0.01266)
r3 = (0, 0, 0, 1)
```

---

# 14. 現時点で分かっていること

かなり重要：

### かなり無罪になったもの

* Vertex Input layout
* BLENDINDICES
* BLENDWEIGHT
* shaderのindex参照
* shaderのweight blend
* HLSL `mul(skin, position)`
* bgfxの明示transpose不足説
* 今回のモデルのscene root合成順
* CPUのnode hierarchyにおける「一部jointだけ逆yaw」説

少なくともVTX 12400についてはGPU shaderが論理的に正常にskinningしている可能性が高い。

---

# 15. まだ未確定の最重要点

## A. CPU paletteとGPU paletteが完全一致しているか

まだ、

```cpp
jointMtx[6]
jointMtx[15]
```

とRenderDocの、

```text
cb0[28..31]
cb0[64..67]
```

を**同一frameで直接比較していない**。

これをやるべき。

一致すれば、

```text
CPU global
→ CPU skin matrix
→ bgfx setUniform
→ D3D11 constant buffer
```

までほぼ確定で正常。

---

## B. skin後のvertex positionをShader Debuggerで直接確認する

shaderの、

```text
82: dp4 r0.x ...
83: dp4 r0.y ...
84: dp4 r0.z ...
85: dp4 r0.w ...
```

直後の `r0` を見る。

yaw=90°では理論上、

```text
skinned position ≈
(0.04858, 0.97052, -0.01267, 1)
```

になる。

※この値は提示されたbone matrixとweightから計算したもの。

もしRenderDocのShader Debuggerでも一致するなら、

```text
vertex input
→ bone palette
→ skinning
→ skinned position
```

が確定。

---

# 16. 次にやるべき調査

優先順位：

### ① CPU paletteをログ

同一frame / yaw=90°で、

```cpp
jointMtx[6]
jointMtx[15]
```

を16 floatずつ出す。

RenderDoc：

```text
cb0[28..31]
cb0[64..67]
```

と比較。

---

### ② RenderDocでskin直後を確認

Shader Debuggerで85行目直後：

```text
r0
```

を確認。

期待値：

```text
≈ (0.04858, 0.97052, -0.01267, 1)
```

---

### ③ 別部位のvertexを調べる

今回のVTX 12400だけでなく、

* 頭
* 胴体
* 脚

など、**実際に逆方向へ開いている部位から1頂点ずつ**選ぶ。

各頂点について、

```text
POSITION
BLENDINDICES
BLENDWEIGHT
palette
skin後position
SV_Position
```

を比較。

これが非常に重要。

もし、

```text
head   正常
body   正常
leg    異常
```

などが出れば、全体Transformではなく、

* meshごとのskin
* inverseBind
* bone remap
* node ↔ joint対応
* meshごとのpalette生成

あたりが有力になる。

---

# 17. Pixel Shaderは今のところかなり無罪

確認したPixel Shaderは、

```text
sample texture
→ output color
```

程度。

したがって、

> 「部位が逆方向へ開く」

という幾何学的問題をPixel Shaderが作っている可能性はかなり低い。

---

# 18. 重要な注意

以前の回答では、

> `r0` = skin後position

と説明してしまったが、これは正確ではなかった。

このshaderでは、

```text
82～85
```

がskin後positionを生成し、

```text
86～92
```

でMVPを掛け、

```text
94
mov o0, r0
```

となる。

したがって最終的な `r0` は**SV_Position**。

今後ここを混同しない。

---

# 19. 最終的な現在の仮説

現時点では、

```text
MotionCapture
	↓
joint/global transform
	↓
skin matrix
	↓
GPU palette
	↓
shader skinning
```

の大部分が正常そう。

そのため、当初の

> 「glTFのnode hierarchyそのものが壊れていて、head/body/legが逆yawになる」

という単純な説は弱くなっている。

現在もっと疑わしいのは、

```text
・meshごとのjoint / inverseBind / remap
・複数mesh間でのpalette対応
・glTF skinのjoint対応
・renderer側のmesh単位のtransform
・CPU palette生成時のnodeIdx / jointIdx対応
・skin以外の描画transform
```

など。

特に**「一部の身体パーツだけが逆方向に開く」**という症状なので、全身共通のyaw処理より、**meshごと・skinごとに異なる情報を持っている部分**を重点的に調査する価値が高い。

---

## 次チャットでの最初の一手

まずこれをやる：

```text
1. yaw=90°の同一frameでCPUのjointMtx[6], jointMtx[15]をログ
2. RenderDocのcb0[28..31], cb0[64..67]と比較
3. RenderDoc Shader Debuggerで85行目直後のr0を確認
4. その後、逆方向に開いている別部位のvertexを1つ選ぶ
```

**ここからは「推測でコードを直す」のではなく、CPU → GPU → rasterizerのどこで最初に期待値からズレるかを潰していく。**

これが現在の調査状況。
