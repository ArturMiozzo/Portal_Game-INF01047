#version 330 core
in vec3 cor_v;
in vec4 position_model;
uniform sampler2D TexturePortalGun;

uniform int object_id;

uniform vec4 bbox_min;
uniform vec4 bbox_max;

out vec3 color;

#define PORTALGUN 0
void main()
{
    vec3 Kd0;
    if(true);
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        float U = (position_model.x-minx)/(maxx-minx);
        float V = (position_model.y-miny)/(maxy-miny);

        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        Kd0 = texture(TexturePortalGun, vec2(U,V)).rgb;

    }
    color = Kd0 * cor_v;
}
