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
#define block_size 8
#define neighbor 8

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
    int x, y;
}Vector2;

Vector2 **Ra, **Rv;
int currentCorrespondence=0;
int maxCorrespondence=2000;
int fullFrameSize=video_width*video_height+video_width*video_height/2;

//Primeiro frame, usado como referencia
unsigned char reference [video_height][video_width];
unsigned char current   [video_height][video_width];

int currentFrame = 0;

int main(int argc, char** argv){
    //Criando os vetores de referencia
    Ra = malloc(video_frames*sizeof(Vector2 *));
    Rv = malloc(video_frames*sizeof(Vector2 *));

    //Busca o frame de referencia
    GetFrame(currentFrame++);

    //Percorre cada frame e verifica seus respectivos blocos
    //#pragma omp parallel for private(current)
    for (int frame = 1; frame <= video_frames; frame++)
    {
        Ra[frame] = malloc(maxCorrespondence*sizeof(Vector2));
        Rv[frame] = malloc(maxCorrespondence*sizeof(Vector2));
        GetFrame(currentFrame++);
        //Verifica em cada pixel de cada frame (CURRENT) se 
        //existe um bloco identico na REFERENCIA
        //#pragma omp parallel for
        for (int i = 0; i < video_height; i+=8) // Linha
        {
            #pragma omp parallel for        
            for (int j = 0; j < video_width; j+=8) //Coluna
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
    #pragma omp parallel for
    for (int i = MAX(x-neighbor, 0); i < MIN(video_height, x+neighbor); i++)
    {
        for (int j = MAX(y-neighbor, 0); j < MIN(video_width, y+neighbor); j++)
        {
            if(reference[x][y] == current[i][j])
            {
                //Encontrou um pixel que é igual na referencia e no atual
                //printf("Buscando no elemento %d\n",i*video_height+j);
                int hasBlock = ComparePixel(i, j);
                if(hasBlock!=-1){
                    printf("Um bloco foi achado no frame %d, com posição [%d,%d] e deslocamento [%d,%d] em relação ao primeiro frame\n",frame,j,i,i-x,j-y);
                    //Já achou este bloco, pode passar para o próximo
                    
                    Vector2 va,vb;
                    va.x=i;
                    va.y=j;
                    vb.x=x-i;
                    vb.y=y-j;
                    InsertCorrespondence(frame,va,vb);
                    
                    j+=8;
                }
            }
        }        
    }

    return -1;//Não existe       
}

int ComparePixel(int x, int y){

    //Não usei omp por serem poucas chamadas no for
    for (int i = x-neighbor; i < x+neighbor; i++)
    {
        if(i<0)
            i=0;
        if(i>video_height)
            i=x+neighbor;
        for (int j = y-neighbor; j < y+neighbor; j++)
        {
            if(j<0)
                j=0;
            if(j>video_width)
                j=y+neighbor;
            if(reference[i][j] == current[x][y]){
                if(VerifyBlock(x,y,i,j)){
                    //Mesmo Bloco
                    printf("É o mesmo Bloco!! [%d %d]  [%d %d]",i,j,x,y);
                    return i*video_height+j;
                }
            }
        }        
    }
}

int VerifyBlock(int x, int y, int rx, int ry){
    int isOk=1;
    for (int i = x; i < x+block_size; i++)
    {
        for (int j = y; j < y+block_size; j++)
        {
            if(reference[i][j] != current[x][y]){
                isOk=0;
                break;
            }
        }     
        if(!isOk) 
            break;  
    }
    return isOk;
}

void GetFrame(int frame_num)
{
    printf("Buscando  o frame %d\n",frame_num);

    FILE *file = fopen("original.yuv","rb+");
        int nSize = video_width*video_height;
        if(!file){
            printf("Erro ao abrir arquivo");
            return -1;
        }
    unsigned char ignore [video_height*video_width/2];

    //Desloca para o frame desejado através do cálculo:
    //tamanho do frame*número de frames pulados*tamanho do char
    //Apartir do início do arquivo
    fseek(file, fullFrameSize*frame_num*sizeof(char), SEEK_SET);

    fread(frame_num==0?reference:current, sizeof(unsigned char), (video_width * video_height), file);   

    printf("O frame %d já foi colocado no vetor %s\n", frame_num, frame_num==0?"referencia":"atual");

    return NULL;
}

void InsertCorrespondence(int frame, Vector2 ref, Vector2 cur){
    if(currentCorrespondence+1>=maxCorrespondence){
        maxCorrespondence*=2;
        Ra[frame] = (Vector2*)realloc(Ra, maxCorrespondence*sizeof(Vector2 *));
        Rv[frame] = (Vector2*)realloc(Rv, maxCorrespondence*sizeof(Vector2 *));
    }

    Ra[frame][currentCorrespondence]=ref;
    Rv[frame][currentCorrespondence]=cur;

    currentCorrespondence++;
}