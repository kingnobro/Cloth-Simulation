## Cloth

存放 `dxf` 格式的衣片

使用 `dxflib` 解析文件



#### woman-shirt.dxf

- 用 ABViewer 打开 `woman-shirt.dxf` 文件，观察发现有五块衣片，并且每块衣片中都有一根竖线
- 用文本编辑器打开 `woman-shirt.dxf` 文件，观察发现有五个 `POLYLINE` 和五根 `LINE`。`POLYLINE` 出现在衣片的起始位置，`LINE` 出现在结尾位置。这个 `LINE` 其实是（每块衣片都有的）丝缕线，它用于指明，当机器裁剪衣服的时候，衣服放置的方向。考虑用 `POLYLINE` 来唯一标识一块衣片。
- **解析方法**：每次解析到一个 `VERTEX` 就累加顶点数量，并且将顶点存储到数组中，遇到下一个 `POLYLINE` 时表示一块衣片的顶点已经读完，创建一片衣片即可
- 解析出的数据：所有 `Vertex` 构成衣片的轮廓线；`Point` 数据不知道是什么，看似像缝合线但实际上不是；
- 如何把衣片上的点分段？Vertex 是顺序排列的，所以对于任意连续的三个点 `prev`, `middle`, `next`，我们可以求出两个向量 $n_1=middle-prev, n_2=next-middle$，利用向量夹角公式求出余弦值。如果两个向量方向相同且余弦值大于某个 threshold，那么它就不是拐点，否则就被认为是拐点。拐点把衣片分成若干 segments