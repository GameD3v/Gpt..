// CommonN3Structures.h
#pragma once

#include <DirectXMath.h> // DirectX::XMFLOAT3 için

// N3VMesh'in Load fonksiyonundan okunacak pozisyon yapısı
struct __Vector3
{
    float x, y, z;

    // DirectX::XMFLOAT3'e dönüşüm operatörü (D3D11Renderer'da direkt kullanılabilir)
    operator DirectX::XMFLOAT3() const { return DirectX::XMFLOAT3(x, y, z); }
};

// Shader'ın beklediği ve DirectX 11 buffer'ına gidecek vertex yapısı
// Orijinal FVF_CV'ye uygun: Pozisyon (float3) + Renk (float4)
struct __VertexColor
{
    float x, y, z;          // POSITION
    unsigned int color;     // COLOR (0xAARRGGBB formatında)

    // Yapıcı
    __VertexColor() = default;
    __VertexColor(float fx, float fy, float fz, unsigned int c) : x(fx), y(fy), z(fz), color(c) {}
};