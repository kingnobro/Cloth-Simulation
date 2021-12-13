#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Cloth.h"
#include "Camera.h"

class ClothPicker
{
public:
    ClothPicker(Camera* cam)
    {
        camera = cam;
    }

    /*
     * check whether ray intersect with cloths
     * return the cloth with the greatest z
     */
    Cloth* pickCloth(const vector<Cloth*>& cloths, const glm::vec3& ray)
    {
        // points on the cloth
        Cloth* selectedCloth = nullptr;
        // distance between camera and hitpoint
        float distance;
        glm::vec3 hitPoint;

        for (Cloth* cloth : cloths)
        {
            // upgrade: ���ѡ�в�������״?
            glm::vec3 pointLeftUpper = cloth->clothPos;
            glm::vec3 pointRightUpper = pointLeftUpper + glm::vec3(cloth->width, 0, 0);
            glm::vec3 pointRightBottom = pointLeftUpper + glm::vec3(cloth->width, -cloth->height, 0);

            // �ռ���ֱ����ƽ�潻�㹫ʽ
            // -------------------------
            // normal vector of the cloth
            glm::vec3 normal = glm::cross(pointRightBottom - pointLeftUpper, pointRightUpper - pointLeftUpper);
            // hitPoint = camera.Position + ray * t
            double t = glm::dot(pointLeftUpper - camera->Position, normal) / glm::dot(ray, normal);
            // intersection point
            glm::vec3 hit = camera->Position + glm::vec3(ray.x * t, ray.y * t, ray.z * t);
            if (hit.x >= pointLeftUpper.x && hit.x <= pointRightUpper.x && hit.y <= pointLeftUpper.y && hit.y >= pointRightBottom.y)
            {
                float d = glm::length(hit - camera->Position);
                // �������Ƭ�ص�ʱ, ѡ���������������Ƭ
                if (selectedCloth == nullptr || (selectedCloth != nullptr && d < distance)) {
                    selectedCloth = cloth;
                    distance = d;
                    hitPoint = hit;
                }
            }
        }
        if (selectedCloth != nullptr) {
            std::cout << "Cloth " << selectedCloth->GetClothID() << " Selected\n";

            // todo: select sewing line
            // �û����һ�㣬������õ�������������ϵĵ���뵽������б�
            // ------------------------------------------
            // �ϡ��¡����� �����ߵ��е�
            // int width = selectedCloth->width;
            // int height = selectedCloth->height;
            // glm::vec3 clothPos = selectedCloth->clothPos;
            // glm::vec3 topMiddle = clothPos + glm::vec3(width / 2.0f, 0, 0);
            // glm::vec3 bottomMiddle = clothPos + glm::vec3(width / 2.0f, -height, 0);
            // glm::vec3 leftMiddle = clothPos + glm::vec3(0, -height / 2, 0);
            // glm::vec3 rightMiddle = clothPos + glm::vec3(width, -height / 2, 0);
            // 
            // // ���ñ�Ե�ϵĵ���뵽 sewNode ��
            // const float d1 = glm::distance(hitPoint, topMiddle);
            // const float d2 = glm::distance(hitPoint, bottomMiddle);
            // const float d3 = glm::distance(hitPoint, leftMiddle);
            // const float d4 = glm::distance(hitPoint, rightMiddle);
            // if (d1 < d2 && d1 < d3 && d1 < d4) {
            //     for (int i = 0; i < selectedCloth->nodesPerRow; i++) {
            //         selectedCloth->sewNode.push_back(selectedCloth->getNode(i, 0));
            //     }
            // }
            // if (d2 < d1 && d2 < d3 && d2 < d4) {
            //     for (int i = 0; i < selectedCloth->nodesPerRow; i++) {
            //         selectedCloth->sewNode.push_back(selectedCloth->getNode(i, selectedCloth->nodesPerCol - 1));
            //     }
            // }
            // if (d3 < d1 && d3 < d2 && d3 < d4) {
            //     for (int i = 0; i < selectedCloth->nodesPerCol; i++) {
            //         selectedCloth->sewNode.push_back(selectedCloth->getNode(0, i));
            //     }
            // }
            // if (d4 < d1 && d4 < d2 && d4 < d3) {
            //     for (int i = 0; i < selectedCloth->nodesPerCol; i++) {
            //         selectedCloth->sewNode.push_back(selectedCloth->getNode(selectedCloth->nodesPerRow - 1, i));
            //     }
            // }

        }
        return selectedCloth;
    }
private:
    Camera* camera;
};