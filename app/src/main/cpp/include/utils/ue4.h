#ifndef UE4_H
#define UE4_H

#include <math.h>
#include <vector>
#include "memory.h"
#include "utils/structs.h"

namespace Ue4 {

    Structs::FMatrix rotator_to_matrix(Structs::FRotator rotation) {
        float radPitch = rotation.Pitch * ((float) 3.14159265358979323846 / 180.0f);
        float radYaw = rotation.Yaw * ((float) 3.14159265358979323846 / 180.0f);
        float radRoll = rotation.Roll * ((float) 3.14159265358979323846 / 180.0f);

        float SP = sinf(radPitch);
        float CP = cosf(radPitch);
        float SY = sinf(radYaw);
        float CY = cosf(radYaw);
        float SR = sinf(radRoll);
        float CR = cosf(radRoll);

        Structs::FMatrix matrix;

        matrix.M[0][0] = (CP * CY);
        matrix.M[0][1] = (CP * SY);
        matrix.M[0][2] = (SP);
        matrix.M[0][3] = 0;

        matrix.M[1][0] = (SR * SP * CY - CR * SY);
        matrix.M[1][1] = (SR * SP * SY + CR * CY);
        matrix.M[1][2] = (-SR * CP);
        matrix.M[1][3] = 0;

        matrix.M[2][0] = (-(CR * SP * CY + SR * SY));
        matrix.M[2][1] = (CY * SR - CR * SP * SY);
        matrix.M[2][2] = (CR * CP);
        matrix.M[2][3] = 0;

        matrix.M[3][0] = 0;
        matrix.M[3][1] = 0;
        matrix.M[3][2] = 0;
        matrix.M[3][3] = 1;

        return matrix;
    }

    Structs::FVector
    world_to_screen(Structs::FVector worldLocation, Structs::MinimalViewInfo camViewInfo,
                    int screenWidth, int screenHeight) {
        Structs::FMatrix tempMatrix = rotator_to_matrix(camViewInfo.Rotation);

        Structs::FVector vAxisX(tempMatrix.M[0][0], tempMatrix.M[0][1], tempMatrix.M[0][2]);
        Structs::FVector vAxisY(tempMatrix.M[1][0], tempMatrix.M[1][1], tempMatrix.M[1][2]);
        Structs::FVector vAxisZ(tempMatrix.M[2][0], tempMatrix.M[2][1], tempMatrix.M[2][2]);

        Structs::FVector vDelta = worldLocation - camViewInfo.Location;

        Structs::FVector vTransformed(Structs::FVector::Dot(vDelta, vAxisY),
                                      Structs::FVector::Dot(vDelta, vAxisZ),
                                      Structs::FVector::Dot(vDelta, vAxisX));

        float fov = camViewInfo.FOV;
        float screenCenterX = (screenWidth / 2.0f);
        float screenCenterY = (screenHeight / 2.0f);

        float X = (screenCenterX + vTransformed.X * (screenCenterX / tanf(fov *
                                                                          ((float) 3.14159265358979323846 /
                                                                           360.0f))) /
                                   vTransformed.Z);
        float Y = (screenCenterY - vTransformed.Y * (screenCenterX / tanf(fov *
                                                                          ((float) 3.14159265358979323846 /
                                                                           360.0f))) /
                                   vTransformed.Z);
        float Z = vTransformed.Z;

        return {X, Y, Z};
    }

    Structs::FMatrix matrix_multiplication(Structs::FMatrix m1, Structs::FMatrix m2) {
        Structs::FMatrix matrix = Structs::FMatrix();
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    matrix.M[i][j] += m1.M[i][k] * m2.M[k][j];
                }
            }
        }
        return matrix;
    }

    Structs::FMatrix transform_to_matrix(Structs::FTransform transform) {
        Structs::FMatrix matrix;

        matrix.M[3][0] = transform.Translation.X;
        matrix.M[3][1] = transform.Translation.Y;
        matrix.M[3][2] = transform.Translation.Z;

        float x2 = transform.Rotation.X + transform.Rotation.X;
        float y2 = transform.Rotation.Y + transform.Rotation.Y;
        float z2 = transform.Rotation.Z + transform.Rotation.Z;

        float xx2 = transform.Rotation.X * x2;
        float yy2 = transform.Rotation.Y * y2;
        float zz2 = transform.Rotation.Z * z2;

        matrix.M[0][0] = (1.0f - (yy2 + zz2)) * transform.Scale3D.X;
        matrix.M[1][1] = (1.0f - (xx2 + zz2)) * transform.Scale3D.Y;
        matrix.M[2][2] = (1.0f - (xx2 + yy2)) * transform.Scale3D.Z;

        float yz2 = transform.Rotation.Y * z2;
        float wx2 = transform.Rotation.W * x2;
        matrix.M[2][1] = (yz2 - wx2) * transform.Scale3D.Z;
        matrix.M[1][2] = (yz2 + wx2) * transform.Scale3D.Y;

        float xy2 = transform.Rotation.X * y2;
        float wz2 = transform.Rotation.W * z2;
        matrix.M[1][0] = (xy2 - wz2) * transform.Scale3D.Y;
        matrix.M[0][1] = (xy2 + wz2) * transform.Scale3D.X;

        float xz2 = transform.Rotation.X * z2;
        float wy2 = transform.Rotation.W * y2;
        matrix.M[2][0] = (xz2 + wy2) * transform.Scale3D.Z;
        matrix.M[0][2] = (xz2 - wy2) * transform.Scale3D.X;

        matrix.M[0][3] = 0;
        matrix.M[1][3] = 0;
        matrix.M[2][3] = 0;
        matrix.M[3][3] = 1;

        return matrix;
    }

    Structs::FVector matrix_to_vector(Structs::FMatrix matrix) {
        return {matrix.M[3][0], matrix.M[3][1], matrix.M[3][2]};
    }



    Structs::FVector transform_to_location(Structs::FTransform c2w, Structs::FTransform transform) {
        return matrix_to_vector(
                matrix_multiplication(transform_to_matrix(transform), transform_to_matrix(c2w)));
    }

} // namespace Ue4

#endif // UE4_H