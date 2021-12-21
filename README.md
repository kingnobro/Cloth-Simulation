# Naive-Cloth-Sewing

### Cloth Sewing

<img src="https://s3.bmp.ovh/imgs/2021/12/7ecf29a3095153af.gif" style="zoom: 50%;" />



### Triangulation

<img src="https://s3.bmp.ovh/imgs/2021/12/8554f5923b5141a5.png" style="zoom: 67%;" />



### Implementation

1. Cloth Simulation based on Mass-Spring System
2. High Efficiency Collision Detection
3. Constrained Delaunay Triangulation
4. Parsing Cloth Data from `.dxf` and Sewing them together.



### Build

You need to make following modifications in Visual Studio,

- INCLUDE
- LIB
- Additional Dependencies: `glfw3.lib`, `assimp-vc142-mt.lib`
- Preprocessor Definitions: `_CRT_SECURE_NO_WARNINGS`



### Future Work

- Cloth Self Collision
- Multilayer Cloth Simulation



### Reference

1. [Cloth Simulation](https://github.com/xxMeow/ClothSimulation)
2. [CDT: Constrained Delaunay Triangulation](https://github.com/artem-ogre/CDT)
3. [Efficient Cloth Model and Collision Detection for Dressing Virtual People](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.103.4540&rep=rep1&type=pdf)
4. [dxflib](https://qcad.org/en/90-dxflib)
5. [assimp](https://www.assimp.org/)

