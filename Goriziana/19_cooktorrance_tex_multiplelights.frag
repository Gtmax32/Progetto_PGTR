/*
19_cooktorrance_tex_multiplelights.frag: come 17_cooktorrance_tex.frag, ma con supporto a luci multiple

NB1) usare 18_phong_tex_multiplelights.vert come vertex shader
NB2) in questo shader sono considerate solo pointlights, andrebbe modificato in caso di + luci di natura diversa 
NB3) ci sono metodi + efficienti per passare dati multipli agli shader (guardare Uniform Buffer Objects)
NB4) Solo con le ultime versioni di OpenGL, tramite strutture come quelle accennate sopra, è possibile passare un numero dinamico di luci

autore: Davide Gadia

Programmazione Grafica per il Tempo Reale - a.a. 2015/2016
C.d.L. Magistrale in Informatica
Universita' degli Studi di Milano

*/

#version 330 core

// numero di luci nella mia scena
#define NR_LIGHTS 3

const float PI = 3.14159;


// variabile di output dello shader
out vec4 colorFrag;

// vettori di incidenza della luce (calcolato nel vertex shader)
in vec3 lightDirs[NR_LIGHTS];
// normale (in coordinate vista)
in vec3 vNormal;
//vettore da vertice a camera (in coordinate vista)
in vec3 vViewPosition;

// il valore è stato calcolato per-vertex nel vertex shader
in vec2 interp_UV;

//uniform che indica il numero di ripetizioni della texture
uniform float repeat;

// uniform che indica la texture da utilizzare
uniform sampler2D tex;

uniform float m; // rugosità superficie - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence
uniform float Kd; // fraction of diffuse reflection (specular reflection = 1 - k)
    


void main()
{
    // applico la ripetizione delle UV e campiono la texture
    vec2 repeated_Uv = mod(interp_UV*repeat, 1.0);
    vec4 surfaceColor = texture(tex, repeated_Uv);

    // normalizzo la normale    
    vec3 N = normalize(vNormal);

    // inizializzo il colore finale
    vec4 color = vec4(0.0,0.0,0.0,1.0);
    
    
    //per tutte le luci nella scena
    for(int i = 0; i < NR_LIGHTS; i++)
    {
        // normalizzo il vettore di incidenza della luce
        vec3 L = normalize(lightDirs[i].xyz);

        // calcolo del coefficiente del modello di Lambert
        float lambertian = max(dot(L,N), 0.0);

        float specular = 0.0;
        
        // se la componente lambertiana è positiva, procedo al calcolo della componente speculare
        if(lambertian > 0.0)
        {
            // il vettore di vista era stato calcolato nel vertex shader, e il valore era stato già negato per avere il verso dal punto alla camera.
            vec3 V = normalize( vViewPosition );

            // calcolo del Half Vector
            vec3 H = normalize(L + V);
           
            // implementazione delle formule viste nelle slide
            // spezzo in componenti

            // preparo gli angoli e i parametri che mi serviranno per il calcolo delle varie componenti
            float NdotH = max(dot(N, H), 0.0); 
            float NdotV = max(dot(N, V), 0.0); 
            float VdotH = max(dot(V, H), 0.0);
            float mSquared = m * m;
            
            // Attenuazione geometrica G
            float NH2 = 2.0 * NdotH;
            float g1 = (NH2 * NdotV) / VdotH;
            float g2 = (NH2 * lambertian) / VdotH;
            float geoAtt = min(1.0, min(g1, g2));

            // Rugosità D
            // Distribuzione di Beckmann
            // posso semplificare la tangente all'esponente cosi':
            // tan = sen/cos -> tan^2 = sen^2/cos^2 -> tan^2 = (1-cos^2)/cos^2
            // l'esponente diventa quindi -(1-cos^2)/(m^2*cos^2) -> (cos^2-1)/(m^2*cos^2)
            float r1 = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
            float r2 = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
            float roughness = r1 * exp(r2);
            
            // Riflettanza di Fresnel F (approx Schlick)
            float fresnel = pow(1.0 - VdotH, 5.0);
            fresnel *= (1.0 - F0);
            fresnel += F0;
            
            // metto tutto assieme per la componente speculare
            specular = (fresnel * geoAtt * roughness) / (NdotV * lambertian * PI);
            
            color += surfaceColor * lambertian * (Kd + specular * (1.0 - Kd));

        }

    }
    
    // calcolo colore finale considerando il colore preso dalla texture
    colorFrag  = color;
}