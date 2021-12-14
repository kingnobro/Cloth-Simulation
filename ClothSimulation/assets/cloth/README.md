## Cloth

存放 `dxf` 格式的衣片

使用 `dxflib` 解析文件



#### woman-shirt.dxf

- 用 ABViewer 打开 `woman-shirt.dxf` 文件，观察发现有五块衣片，并且每块衣片中都有一根竖线

- 用文本编辑器打开 `woman-shirt.dxf` 文件，观察发现有五个 `POLYLINE` 和五根 `LINE`。`POLYLINE` 出现在衣片的起始位置，`LINE` 出现在结尾位置。所以考虑用 `LINE` 来唯一标识一块衣片。这个 `LINE` 其实是（每块衣片都有的）丝缕线，它用于指明，当机器裁剪衣服的时候，衣服放置的方向
- **解析方法**：每次解析到一个 `VERTEX` 或者 `POINT` 就累加顶点数量，并且将顶点存储到数组中，遇到 `LINE` 时表示一块衣片的顶点已经读完，创建一片衣片即可