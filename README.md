# Naive-Cloth-Sewing

<img src="https://s3.bmp.ovh/imgs/2021/12/7ecf29a3095153af.gif" style="zoom: 50%;" />

### Characteristic

1. cloth Simulation based on mass spring system
2. $O(1)$ collision detection
3. Constrained Delaunay Triangulation
4. `.dxf` parser using `dxflib`
5. model import using `assimp` library
6. click and select

### Build

需要在 Visual Studio 中配置的属性

- INCLUDE 设置为项目的 `includes` 路径
- LIB，设置为项目的 `lib` 路径
- 链接器/输入/附加依赖项，添加 `glfw3.lib` 和 `assimp-vc142-mt.lib`（如果 assimp 的 dll 库运行失败，可能需要自己重新下载源码并在本地机编译）
- 预处理加上标志 `_CRT_SECURE_NO_WARNINGS`

### Reference

- [ClothSimulation](https://github.com/xxMeow/ClothSimulation)
