#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define FLOOR 0
#define WALL  1
#define ROOF  2
#define PORTALGUN 3

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureFloor;
uniform sampler2D TextureWall;
uniform sampler2D TextureRoof;
uniform sampler2D TexturePortalGun;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    vec3 Kd0;

    if ( object_id == FLOOR )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*5 - floor(texcoords.x*5);
        V = texcoords.y*5 - floor(texcoords.y*5);
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd0 = texture(TextureFloor, vec2(U,V)).rgb;
    }
    else if ( object_id == WALL )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*5 - floor(texcoords.x*5);
        V = texcoords.y*2 - floor(texcoords.y*2);
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd0 = texture(TextureWall, vec2(U,V)).rgb;
    }
    else if ( object_id == ROOF )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x*5 - floor(texcoords.x*5);
        V = texcoords.y*5 - floor(texcoords.y*5);
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd0 = texture(TextureRoof, vec2(U,V)).rgb;
    }
    else if ( object_id == PORTALGUN )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x-minx)/(maxx-minx);
        V = (position_model.y-miny)/(maxy-miny);

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd0 = texture(TexturePortalGun, vec2(U,V)).rgb;
    }

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    color = Kd0;// * (lambert + 0.01);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec3(1.0,1.0,1.0)/2.2);
}

