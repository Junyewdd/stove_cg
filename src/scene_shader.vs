#version 330 core
layout (location = 0) in vec3 aPos;       // 정점 위치
layout (location = 1) in vec3 aNormal;    // 법선
layout (location = 2) in vec2 aTexCoords; // 텍스처 좌표

out vec3 WorldPos;       // 월드 좌표
out vec3 Normal;        // 변환된 법선
out vec2 TexCoords;     // 텍스처 좌표

uniform mat4 model;     // 모델 행렬
uniform mat4 view;      // 뷰 행렬
uniform mat4 projection; // 투영 행렬

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));             // 월드 공간 좌표
    Normal = mat3(transpose(inverse(model))) * aNormal;   // 월드 공간 법선
    TexCoords = aTexCoords;                               // 텍스처 좌표 전달
    gl_Position = projection * view * vec4(WorldPos, 1.0); // 클립 공간 좌표
}
