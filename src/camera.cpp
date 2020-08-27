#include <GLFW/glfw3.h>
#include <GLUT/glut.h>
#include <map>
#include "../include/camera.h"
#include "../include/scene.h"
#define NEAR_CLIP 0.01f
#define FAR_CLIP 1000
void Camera::render() {
    auto& objects = scene->objects;
    rotationMatrix(-rotation.x, -rotation.y, -rotation.z, -rotation.w, A);
    Vector4 norm3d;// 用于存储投影体的某个面3d法线
    for (auto object : objects) {
        auto objpos = object->position;
        auto& points = object->points;
        auto& bodys = object->bodys;
        objpos -= position;
        objpos.rotate(A);
//        if (objpos.w < NEAR_CLIP || objpos.w > FAR_CLIP) continue;
        Vector4 TPoints[object->points.size()];
        Vector4 NMAPTPoints[object->points.size()];
        Vector4* tpoint = TPoints, *nmtp = NMAPTPoints;
        for (auto point : points) {
            tpoint->operator=(*point);
            tpoint->operator+=(objpos);
            tpoint->rotate(A);
            nmtp->operator=(*tpoint);
            tpoint->x /= tpoint->w * TAN_hfov_x;
            tpoint->y /= tpoint->w * TAN_hfov_y;
            tpoint->z /= tpoint->w * TAN_hfov_z;
            tpoint++, nmtp++;
        }
//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        float c = 0;
        float color = 0.1;
        const float step = .99f * .25f / bodys.size();
        unsigned npoint3[3], notnpoint3[3];
        GLfloat tmpPosition3d[3];
        for (auto body : bodys) {
//            if (objpos * *body->n > 0) continue;
//            if ((points[body->points[0]])->w < 0 || (points[body->points[1]])->w < 0 || (points[body->points[2]])->w < 0 || (points[body->points[3]])->w < 0) continue;
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < 2; i++) {
                for (int j = i + 1; j < 3; j++) {
                    for (int k = j + 1; k < 4; k++) {
                        auto p1 = TPoints + body->points[i], p2 = TPoints + body->points[j], p3 = TPoints + body->points[k];
                        // 做裁剪
                        unsigned* t = npoint3;
                        unsigned* tmp = notnpoint3;
                        char len = 0;
                        if (p1->w < NEAR_CLIP) *(t++) = body->points[i], len++; else *(tmp++) = body->points[i];
                        if (p2->w < NEAR_CLIP) *(t++) = body->points[j], len++; else *(tmp++) = body->points[j];
                        if (p3->w < NEAR_CLIP) *(t++) = body->points[k], len++; else *(tmp++) = body->points[k];
                        // 以上为过近裁面的点的统计
                        // 渲染部分：计算颜色&解决z-fighting
                        // 渲染
                        normal3D(norm3d, *p1, *p2, *p3);
                        norm3d *= 0.001f / norm3d.mod();
                        glColor3f(.1 + norm3d.y * 50, .7 + norm3d.y * 500, .5 + norm3d.y * 200);
                        // 解决z-fighting的偏移操作
                        (TPoints + body->points[i])->operator+=(norm3d);
                        (TPoints + body->points[j])->operator+=(norm3d);
                        (TPoints + body->points[k])->operator+=(norm3d);
                        // 继续进行裁剪与顶点绘制
                        switch (len) {
                            case 0:
                                glVertex3fv((GLfloat*)(TPoints + body->points[i]));
                                glVertex3fv((GLfloat*)(TPoints + body->points[j]));
                                glVertex3fv((GLfloat*)(TPoints + body->points[k]));
                                break;
                            case 1: {
                                auto p = NMAPTPoints[npoint3[0]];
                                Vector4 n1(NMAPTPoints[notnpoint3[0]] - NMAPTPoints[npoint3[0]]),
                                        n2(NMAPTPoints[notnpoint3[1]] - NMAPTPoints[npoint3[0]]);
                                float t1 = (NEAR_CLIP - p.w) / n1.w;
                                float t2 = (NEAR_CLIP - p.w) / n2.w;
                                float invPW1 = 1 / NMAPTPoints[notnpoint3[0]].w;
                                float invPW2 = 1 / NMAPTPoints[notnpoint3[1]].w;
                                float x1((n1.x * t1 + p.x) * invPW1 * iTAN_hfov_x),
                                      y1((n1.y * t1 + p.y) * invPW1 * iTAN_hfov_y),
                                      z1((n1.z * t1 + p.z) * invPW1 * iTAN_hfov_z);
                                float x2((n2.x * t2 + p.x) * invPW2 * iTAN_hfov_x),
                                      y2((n2.y * t2 + p.y) * invPW2 * iTAN_hfov_y),
                                      z2((n2.z * t2 + p.z) * invPW2 * iTAN_hfov_z);

                                tmpPosition3d[0] = x1,
                                tmpPosition3d[1] = y1,
                                tmpPosition3d[2] = z1;
                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                glVertex3fv(tmpPosition3d);

                                glVertex3fv(tmpPosition3d);
                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                tmpPosition3d[0] = x2,
                                tmpPosition3d[1] = y2,
                                tmpPosition3d[2] = z2;
                                glVertex3fv(tmpPosition3d);

                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                glVertex3fv(tmpPosition3d);
                                break;
                            }
                            case 2: {
                                auto p = NMAPTPoints[notnpoint3[0]];
                                Vector4 n1(NMAPTPoints[npoint3[0]] - NMAPTPoints[notnpoint3[0]]),
                                        n2(NMAPTPoints[npoint3[1]] - NMAPTPoints[notnpoint3[0]]);
                                float t1 = (NEAR_CLIP - p.w) / n1.w;
                                float t2 = (NEAR_CLIP - p.w) / n2.w;
                                float invPW1 = 1 / NMAPTPoints[notnpoint3[0]].w;
                                float invPW2 = 1 / NMAPTPoints[notnpoint3[1]].w;
                                float x1((n1.x * t1 + p.x) * invPW1 * iTAN_hfov_x),
                                        y1((n1.y * t1 + p.y) * invPW1 * iTAN_hfov_y),
                                        z1((n1.z * t1 + p.z) * invPW1 * iTAN_hfov_z);
                                float x2((n2.x * t2 + p.x) * invPW2 * iTAN_hfov_x),
                                        y2((n2.y * t2 + p.y) * invPW2 * iTAN_hfov_y),
                                        z2((n2.z * t2 + p.z) * invPW2 * iTAN_hfov_z);
                                tmpPosition3d[0] = x1,
                                tmpPosition3d[1] = y1,
                                tmpPosition3d[2] = z1;
                                glVertex3fv(tmpPosition3d);
                                tmpPosition3d[0] = x2,
                                tmpPosition3d[1] = y2,
                                tmpPosition3d[2] = z2;
                                glVertex3fv(tmpPosition3d);
                                glVertex3fv((GLfloat*)(TPoints + notnpoint3[0]));
                                break;
                            }
                            case 3: break;
                        }
                        // 恢复解决z-fighting的偏移操作
                        (TPoints + body->points[i])->operator-=(norm3d);
                        (TPoints + body->points[j])->operator-=(norm3d);
                        (TPoints + body->points[k])->operator-=(norm3d);
                        color += step;
                    }
                }
            }
//            c += 0.00000001f;
            glEnd();

//            glBegin(GL_LINES);
//            glColor3f(1, 1, 1);
//            for (int i = 0; i < 3; i++) {
//                for (int j = i + 1; j < 4; j++) {
//                    glVertex3fv((GLfloat*)(TPoints + body->points[i]));
//                    glVertex3fv((GLfloat*)(TPoints + body->points[j]));
//                }
//            }
//            glEnd();
        }
    }
}
