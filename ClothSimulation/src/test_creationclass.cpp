#include "test_creationclass.h"

#include <iostream>
#include <stdio.h>


/**
 * Default constructor.
 */
Test_CreationClass::Test_CreationClass() {
    blockCount = 0;
}


/**
 * Sample implementation of the method which handles layers.
 */
void Test_CreationClass::addLayer(const DL_LayerData& data) {
    printf("LAYER: %s flags: %d\n", data.name.c_str(), data.flags);
    printAttributes();
}

/**
 * Sample implementation of the method which handles point entities.
 */
void Test_CreationClass::addPoint(const DL_PointData& data) {
    //printf("POINT    (%6.3f, %6.3f, %6.3f)\n",
    //       data.x, data.y, data.z);
    //printAttributes();
    
    // 忽略 Point 类型的点, 因为发现 dxf 文件中相邻的 Point 在空间上并不是相邻的
    //assert(blockCount > 0);
    //blockNodes[blockCount - 1].push_back(std::make_pair(data.x, data.y));
}

/**
 * Sample implementation of the method which handles line entities.
 */
void Test_CreationClass::addLine(const DL_LineData& data) {
    // 在当前场景下, Line 被用作丝路线, 每片衣片有且仅有一条丝路线, 且存在于顶点数据的末尾
    // 所以, 使用 Line 作为分割衣片数据的标识
    // clothCreator->createCloth();
    //printf("LINE     (%6.3f, %6.3f, %6.3f) (%6.3f, %6.3f, %6.3f)\n",
    //       data.x1, data.y1, data.z1, data.x2, data.y2, data.z2);
    //printAttributes();
}

/**
 * Sample implementation of the method which handles arc entities.
 */
void Test_CreationClass::addArc(const DL_ArcData& data) {
    printf("ARC      (%6.3f, %6.3f, %6.3f) %6.3f, %6.3f, %6.3f\n",
           data.cx, data.cy, data.cz,
           data.radius, data.angle1, data.angle2);
    printAttributes();
}

/**
 * Sample implementation of the method which handles circle entities.
 */
void Test_CreationClass::addCircle(const DL_CircleData& data) {
    printf("CIRCLE   (%6.3f, %6.3f, %6.3f) %6.3f\n",
           data.cx, data.cy, data.cz,
           data.radius);
    printAttributes();
}


/**
 * Sample implementation of the method which handles polyline entities.
 */
void Test_CreationClass::addPolyline(const DL_PolylineData& data) {
    // 每个 POLYLINE 表示一段折线, flag 为 1 表示这是闭合的折线
    // 对于衣片而言, polyline 肯定是闭合的
    blockCount += 1;
    blockNodes.push_back(std::vector<point2D>());
    //printf("POLYLINE \n");
    //printf("flags: %d\n", (int)data.flags);
    //printAttributes();
}


/**
 * Sample implementation of the method which handles vertices.
 */
void Test_CreationClass::addVertex(const DL_VertexData& data) {
    //printf("VERTEX   (%6.3f, %6.3f, %6.3f) %6.3f\n",
    //       data.x, data.y, data.z,
    //       data.bulge);
    //printAttributes();

    // todo: add data.z
    assert(blockCount > 0);
    blockNodes[blockCount - 1].push_back(std::make_pair(data.x, data.y));
}


void Test_CreationClass::add3dFace(const DL_3dFaceData& data) {
    printf("3DFACE\n");
    for (int i=0; i<4; i++) {
        printf("   corner %d: %6.3f %6.3f %6.3f\n", 
            i, data.x[i], data.y[i], data.z[i]);
    }
    printAttributes();
}


void Test_CreationClass::printAttributes() {
    printf("  Attributes: Layer: %s, ", attributes.getLayer().c_str());
    printf(" Color: ");
    if (attributes.getColor()==256)	{
        printf("BYLAYER");
    } else if (attributes.getColor()==0) {
        printf("BYBLOCK");
    } else {
        printf("%d", attributes.getColor());
    }
    printf(" Width: ");
    if (attributes.getWidth()==-1) {
        printf("BYLAYER");
    } else if (attributes.getWidth()==-2) {
        printf("BYBLOCK");
    } else if (attributes.getWidth()==-3) {
        printf("DEFAULT");
    } else {
        printf("%d", attributes.getWidth());
    }
    printf(" Type: %s\n", attributes.getLinetype().c_str());
}
    
    

// EOF
