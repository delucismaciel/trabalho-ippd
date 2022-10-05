#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define BUFFER_SIZE 256
#define video_width 640
#define video_height 360
#define video_frames 119//119
#define neighbor 64

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
    int x, y;
}Vector2;

Vector2 *Ra, *Rv;
int currentCorrespondence=0;
int maxCorrespondence=1000;

//Primeiro frame, usado como referencia
unsigned char reference [video_width*video_height];
unsigned char current   [video_width*video_height];

int currentFrame = 0;

int main(int argc, char** argv){
    //Criando os vetores de referencia
    Ra = malloc(maxCorrespondence*sizeof(Vector2));
    Rv = malloc(maxCorrespondence*sizeof(Vector2));

    FILE *file = fopen("original.yuv","rb+");
	int nSize = video_width*video_height;
    if(!file){
        printf("Erro ao abrir arquivo");
        return -1;
    }

    //Busca o frame de referencia
    GetFrame(file, currentFrame++);

    //Percorre cada frame e verifica seus respectivos blocos
    #pragma omp parallel for private(current)
    for (int frame = 1; frame <= video_frames; frame++)
    {
        GetFrame(file, currentFrame++);
        //Verifica em cada pixel de cada frame (CURRENT) se 
        //existe um bloco identico na REFERENCIA
        #pragma omp parallel for
        for (int i = 0; i < video_height; i++) // Linha
        {
            #pragma omp parallel for        
            for (int j = 0; j < video_width; j++) //Coluna
            {
                FindEqualPixel(i, j, currentFrame);
            }
        }
    }    

    free(Ra);
    free(Rv);

    return 0;
}

int FindEqualPixel(int x, int y, int frame){
    //Verifica entre os NEIGHBORS vizinhos mais próximos,
    //Ignorando os elementos menores do que 0 e maiores do que os limites
    //printf("\nVerificando os pixels do frame %d na thread %d\n",frame, omp_get_thread_num());
    #pragma omp parallel for
    for (int i = MAX(x-neighbor, 0); i < MIN(video_height, x+neighbor); i++)
    {
        for (int j = MAX(y-neighbor, 0); j < MIN(video_width, y+neighbor); j++)
        {
            if(reference[x*video_height+y] == current[i*video_height+j])
            {
                //Encontrou um pixel que é igual na referencia e no atual
                //printf("Buscando no elemento %d\n",i*video_height+j);
                int hasBlock = FindBlock(i, j);
                if(hasBlock!=-1){
                    printf("Um bloco foi achado no frame %d, com posição [%d,%d] e deslocamento [%d,%d] em relação ao primeiro frame\n",frame,j,i,x-i,y-j);
                    //Já achou este bloco, pode passar para o próximo
                    
                    Vector2 va,vb;
                    va.x=i;
                    va.y=j;
                    vb.x=x-i;
                    vb.y=y-j;
                    InsertCorrespondence(va,vb);

                    j+=8;
                }
            }
        }        
    }

    return -1;//Não existe       
}

int FindBlock(int x, int y){
    
    return (x*y%117==0)?1:-1;//Não existe este bloco
}

void GetFrame(FILE *file, int frame_num)
{
    printf("Buscando  o frame %d\n",frame_num);

    unsigned char ignore [video_height*video_width];

    fread(frame_num==0?reference:current, sizeof(unsigned char), (video_width * video_height), file);   
    fread(ignore, sizeof(unsigned char), (video_width * video_height)/4, file);   

    printf("O frame %d já foi colocado no vetor %s\n", frame_num, frame_num==0?"referencia":"atual");

    return NULL;
}

void InsertCorrespondence(Vector2 reference, Vector2 frame){
    if(currentCorrespondence+1>=maxCorrespondence){
        maxCorrespondence*=2;
        Ra = (Vector2*)realloc(Ra, maxCorrespondence*sizeof(Vector2));
        Rv = (Vector2*)realloc(Rv, maxCorrespondence*sizeof(Vector2));
    }

    Ra[currentCorrespondence]=reference;
    Rv[currentCorrespondence]=frame;

    currentCorrespondence++;
}