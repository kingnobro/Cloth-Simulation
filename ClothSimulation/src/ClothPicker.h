#ifndef CLOTH_PICKER_H
#define CLOTH_PICKER_H

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
    Cloth* pickCloth(const std::vector<Cloth*>& cloths, const glm::vec3& ray)
    {
        // points on the cloth
        Cloth* selectedCloth = nullptr;
        // distance between camera and hitpoint
        float distance;
        glm::vec3 hitPoint;

        for (Cloth* cloth : cloths)
        {
            // corners of bounding box
            glm::vec3 pointLeftUpper = cloth->leftUpper;
            glm::vec3 pointRightUpper = cloth->rightUpper;
            glm::vec3 pointRightBottom = cloth->rightBottom;

            // Intersection Point of a Line and a Plane
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
                // when cloths are overlapping, select cloth closest to camera
                if (selectedCloth == nullptr || (selectedCloth != nullptr && d < distance)) {
                    selectedCloth = cloth;
                    distance = d;
                    hitPoint = hit;
                }
            }
        }
        if (selectedCloth != nullptr) {
            std::cout << "Cloth " << selectedCloth->GetClothID() << " Selected\n";

            // find the point 'P' that is closest to the hitpoint on the contour
            // according to P.segmentID, we select all points in that segment
            // ---------------------------------------
            distance = FLT_MAX;
            Node* nearPoint = nullptr;
            for (Node* n : selectedCloth->contour) {
                float d = glm::distance(hitPoint, n->worldPosition);
                if (d < distance && d < 0.2f) {
                    distance = d;
                    nearPoint = n;
                }
            }
            if (nearPoint && !nearPoint->isSelected) {
                int segmentId = nearPoint->segmentID;
                std::vector<Node*> segment;
                for (Node* n : selectedCloth->segments[segmentId]) {
                    if (!n->isSelected) {   // turning point may be selected before
                        segment.push_back(n);
                    }
                    n->isSelected = true;
                }
                selectedCloth->sewNode.push_back(segment);
            }

        }
        return selectedCloth;
    }
private:
    Camera* camera;
};

#endif