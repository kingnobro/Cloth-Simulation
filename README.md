# Cloth-Simulation

Modified from [this repository](https://github.com/xxMeow/ClothSimulation).



### 环境配置

需要在 Visual Studio 中配置的属性

- VC++目录/包含目录（INCLUDE），设置为项目的 `includes` 路径
- VC++目录/库目录（LIB），设置为项目的 `lib` 路径
- 链接器/输入/附加依赖项，添加 `glfw3.lib` 和 `assimp-vc142-mt.lib`
- 如果 assimp 的 dll 库运行失败，你可能需要自己重新下载源码并在本地机编译

