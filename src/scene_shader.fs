#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// ������
uniform vec3 lightPos;
uniform vec3 camPos;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;

const float PI = 3.14159265359;

// �׸��� ���� ���� ������
uniform float shadowIntensity; // 0.0 ~ 1.0 (default: 0.5)

// Fresnel-Schlick �ٻ�
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Geometry Smith
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float ggx2 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx1 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

// GGX Distribution
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// Shadow Calculation
float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // NDC ��ǥ�� ��ȯ
    projCoords = projCoords * 0.5 + 0.5; // [0, 1] ������ ��ȯ

    if (projCoords.z > 1.0)
        return 0.0; // ���� ���� ���̸� �׸��� ����

    float closestDepth = texture(shadowMap, projCoords.xy).r; // �׸��� �� ���� ��
    float currentDepth = projCoords.z;

    // �׸��� ���� ��� (bias �߰�)
    float bias = 0.005;
    float shadow = currentDepth > closestDepth + bias ? 1.0 : 0.0;
    return shadow;
}

void main()
{
    // ���� �Ӽ�
    vec3 albedo = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic = texture(metallicMap, TexCoords).r;
    float roughness = texture(roughnessMap, TexCoords).r;

    // ���� ���
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
    vec3 L = normalize(lightPos - WorldPos);
    vec3 H = normalize(V + L);

    // Fresnel �ݻ� ���
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(max(dot(N, V), 0.0), max(dot(N, L), 0.0), roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    vec3 diffuse = kD * albedo / PI;
    vec3 specular = (NDF * G * F) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001);

    // �׸��� ��� �� ���� �ݿ�
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(WorldPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace);

    // �׸��� ���� ����
    shadow = shadow * shadowIntensity;

    vec3 ambient = vec3(0.45) * albedo;
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular) * NdotL;

    // Gamma ����
    lighting = lighting / (lighting + vec3(1.0));
    lighting = pow(lighting, vec3(1.0 / 2.2));

    FragColor = vec4(lighting, 1.0);
}
