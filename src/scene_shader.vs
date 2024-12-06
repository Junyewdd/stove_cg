#version 330 core
layout (location = 0) in vec3 aPos;       // ���� ��ġ
layout (location = 1) in vec3 aNormal;    // ����
layout (location = 2) in vec2 aTexCoords; // �ؽ�ó ��ǥ

out vec3 WorldPos;       // ���� ��ǥ
out vec3 Normal;        // ��ȯ�� ����
out vec2 TexCoords;     // �ؽ�ó ��ǥ

uniform mat4 model;     // �� ���
uniform mat4 view;      // �� ���
uniform mat4 projection; // ���� ���

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));             // ���� ���� ��ǥ
    Normal = mat3(transpose(inverse(model))) * aNormal;   // ���� ���� ����
    TexCoords = aTexCoords;                               // �ؽ�ó ��ǥ ����
    gl_Position = projection * view * vec4(WorldPos, 1.0); // Ŭ�� ���� ��ǥ
}
