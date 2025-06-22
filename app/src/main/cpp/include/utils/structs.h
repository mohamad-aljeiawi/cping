#ifndef STRUCTS_H
#define STRUCTS_H

#define MAX_NAME_LENGTH 100
#define MAX_OBJECTS 1000

#include <cstdint>
#include <math.h>
#include <string>
#include <vector>

namespace Structs {
    struct FVector {
        float X;
        float Y;
        float Z;

        FVector() : X(0.f), Y(0.f), Z(0.f) {}

        FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}

        float Distance(const FVector &v) const {
            float dx = X - v.X;
            float dy = Y - v.Y;
            float dz = Z - v.Z;
            return sqrt(dx * dx + dy * dy + dz * dz);
        }

        FVector operator-(const FVector &v) const {
            return FVector(X - v.X, Y - v.Y, Z - v.Z);
        }

        FVector operator+(const FVector &v) const {
            return FVector(X + v.X, Y + v.Y, Z + v.Z);
        }

        FVector operator*(float scale) const {
            return FVector(X * scale, Y * scale, Z * scale);
        }

        static float Dot(const FVector &lhs, const FVector &rhs) {
            return lhs.X * rhs.X + lhs.Y * rhs.Y + lhs.Z * rhs.Z;
        }
    };

    struct FVector2 {
        float X;
        float Y;

        FVector2() : X(0.f), Y(0.f) {}

        FVector2(float x, float y) : X(x), Y(y) {}

        float Distance(const FVector2 &v) const {
            float dx = X - v.X;
            float dy = Y - v.Y;
            return sqrt(dx * dx + dy * dy);
        }

        FVector2 operator-(const FVector2 &v) const {
            return FVector2(X - v.X, Y - v.Y);
        }

        FVector2 operator+(const FVector2 &v) const {
            return FVector2(X + v.X, Y + v.Y);
        }

        FVector2 operator*(float scale) const {
            return FVector2(X * scale, Y * scale);
        }

        static float Dot(const FVector2 &lhs, const FVector2 &rhs) {
            return lhs.X * rhs.X + lhs.Y * rhs.Y;
        }
    };

    struct FRotator {
        float Pitch;
        float Yaw;
        float Roll;

        FRotator() : Pitch(0.f), Yaw(0.f), Roll(0.f) {}

        FRotator(float pitch, float yaw, float roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
    };

    struct FQuaternion {
        union {
            struct {
                float X;
                float Y;
                float Z;
                float W;
            };
            float data[4];
        };
    };

    struct FMatrix {
        float M[4][4];
    };

    struct D3DMatrix {
        float _11, _12, _13, _14;
        float _21, _22, _23, _24;
        float _31, _32, _33, _34;
        float _41, _42, _43, _44;
    };

    struct FTransform {
        FQuaternion Rotation;
        FVector Translation;
        char pad[0x4];
        FVector Scale3D;

        D3DMatrix ToMatrixWithScale() {
            D3DMatrix m;
            m._41 = Translation.X;
            m._42 = Translation.Y;
            m._43 = Translation.Z;

            float x2 = Rotation.X + Rotation.X;
            float y2 = Rotation.Y + Rotation.Y;
            float z2 = Rotation.Z + Rotation.Z;

            float xx2 = Rotation.X * x2;
            float yy2 = Rotation.Y * y2;
            float zz2 = Rotation.Z * z2;
            m._11 = (1.0f - (yy2 + zz2)) * Scale3D.X;
            m._22 = (1.0f - (xx2 + zz2)) * Scale3D.Y;
            m._33 = (1.0f - (xx2 + yy2)) * Scale3D.Z;

            float yz2 = Rotation.Y * z2;
            float wx2 = Rotation.W * x2;
            m._32 = (yz2 - wx2) * Scale3D.Z;
            m._23 = (yz2 + wx2) * Scale3D.Y;

            float xy2 = Rotation.X * y2;
            float wz2 = Rotation.W * z2;
            m._21 = (xy2 - wz2) * Scale3D.Y;
            m._12 = (xy2 + wz2) * Scale3D.X;

            float xz2 = Rotation.X * z2;
            float wy2 = Rotation.W * y2;
            m._31 = (xz2 + wy2) * Scale3D.Z;
            m._13 = (xz2 - wy2) * Scale3D.X;

            m._14 = 0.0f;
            m._24 = 0.0f;
            m._34 = 0.0f;
            m._44 = 1.0f;

            return m;
        }

        FVector TransformPosition(FVector position) {
            D3DMatrix m = ToMatrixWithScale();

            return FVector(
                    position.X * m._11 + position.Y * m._21 + position.Z * m._31 + m._41,
                    position.X * m._12 + position.Y * m._22 + position.Z * m._32 + m._42,
                    position.X * m._13 + position.Y * m._23 + position.Z * m._33 + m._43);
        }
    };

    struct MinimalViewInfo {
        FVector Location;
        FVector LocationLocalSpace;
        FRotator Rotation;
        float FOV;
    };

    struct FBoxSphereBounds {
        FVector Origin;
        FVector BoxExtent;
        float SphereRadius;
    };


    struct Request {
        int ScreenWidth;
        int ScreenHeight;
        int ScreenOrientation;
    };

    struct WorldObject {
        char Name[MAX_NAME_LENGTH];
        FVector Location;
        FBoxSphereBounds BoxSphereBounds;
        FTransform Transform;
    };

    struct Response {
        int Count;
        MinimalViewInfo MinimalViewInfo;
        WorldObject Objects[MAX_OBJECTS];
    };

} // namespace Structs

#endif // STRUCTS_H