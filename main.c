#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>

#define BUFFER_SIZE 256
#define video_width 640
#define video_height 360
#define video_frames 119 //119
#define block_size 8
#define neighbor 32 //3x o tamanho do bloco
#define tolerance 6

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
    int l, c;
}Vector2;

typedef struct {
    int l, c;
    int isBlock;
}Offset;

//Primeiro frame, usado como referencia
unsigned char reference [video_height][video_width];
unsigned char current   [video_height][video_width];
//unsigned int qntByFrame [video_frames];

//Offset deslocamento[video_frames][1000];
Offset Ra[video_height/8][video_width/8];
Vector2 Rv[video_height][video_width];

int isBlock[video_height][video_width];
int currentCorrespondence = 0;

int fullFrameSize=video_width*video_height * 3 / 2;
int currentFrame = 0;

int main(int argc, char** argv){
    int selectedFrame = atoi(argv[1]);
    int compareFrame = atoi(argv[2]);
    printf("Cria o vetor de flags");
    int j=0,i=0;

    //  Zera
    #pragma omp parallel for private(j,i)
    for (int index = 0; index < video_height*video_width; index++)
    {
        j=index%video_width;
        i = (int)index/video_width;
        isBlock[i][j]=0;
    }

    //Zera o vetor de resultado
    #pragma omp parallel for collapse(2)
    for (int l = 0; l < video_height/8; l++)
    {
        for (int c = 0; c < video_height/8; c++)
        {
            Ra[l][c].c=0;
            Ra[l][c].l=0;
            Ra[l][c].isBlock=0;
        }
    }

    GetFrame(selectedFrame,1);

    printf("Frames Selecionados: %d e %d\n", selectedFrame, compareFrame);
    printf("Quantidade de Blocos: %d * %d = %d\n", video_height/8, video_width/8, video_height*video_width/64);

    OneFrame(compareFrame);


    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < video_width; j++)
        {
            printf("%d == %d  |", current[i][j],reference[i][j]);
        }
        
    }


    printf("\n\tRa - posições no frame Atual");
    printf("\n\tRv - deslocamento entre o frame atual e a referencia");

    printf("\n----------------------\nTotal de correspondencias: %d\n",currentCorrespondence);

    for (int l = 0; l < video_height/8; l++)
    {
        for (int c = 0; c < video_width/8; c++)
        {
            printf("|  [%3d, %3d] = ", l,c);
            if(Ra[l][c].isBlock){
                printf("%2d,%2d  ", Ra[l][c].l, Ra[l][c].c);                
            }else{
                printf("   X   ");                 

            }
        }
        printf("|\n");
    }
    return 0;
}

void OneFrame(int compareFrame){
    GetFrame(compareFrame,0);
    FrameVerifyByBlock(compareFrame);
}

int GetFrame(int frame_num, int isReference)
{
    //printf("\nBuscando  o frame %d\n",frame_num);

    FILE *file = fopen("original.yuv","rb+");
        int nSize = video_width*video_height;
        if(!file){
            printf("\nErro ao abrir arquivo");
            return -1;
        }

    //Desloca para o frame desejado através do cálculo:
    //tamanho do frame*número de frames pulados*tamanho do char
    //Apartir do início do arquivo
    //printf("\n\tSaltando: %d bytes\n",fullFrameSize*frame_num);
    fseek(file, fullFrameSize*frame_num , SEEK_SET);

    fread(isReference!=0?reference:current, sizeof(unsigned char), (video_width * video_height), file);   

  //  printf("O frame %d já foi colocado no vetor %s\n", frame_num, frame_num==0?"referencia":"atual");

    fclose(file);

    return NULL;
}

void FrameVerifyByBlock(int frame){
    //printf("Verificando o frame %d, na thread %d\n",frame, omp_get_thread_num());
    #pragma omp parallel for collapse(2)
    for (int linha = 0; linha < video_height/8; linha++)
    {
        for (int coluna = 0; coluna < video_width/8; coluna++)
        {
            if(isBlock[linha*8][coluna*8])
                continue;
            //Compara apenas nos vizinhos mais próximos
            FindBlockNeighbor(linha*8, coluna*8);
        }    
    }    
}

void FindBlockNeighbor(int linha, int coluna){
    #pragma omp parallel for collapse(2)
    for (int i = MAX(0, linha-neighbor); i < MIN(video_height, linha + neighbor); i++)
    {
        for (int j = MAX(0,coluna-neighbor); j < MIN(video_width,coluna + neighbor); j++)
        {
            if(isBlock[linha][coluna])
                continue;
                
            //Verifica se são iguais
            int isEqual=1;
            for (int lineOffset = 0; lineOffset < 8; lineOffset++)
            {
                for (int colOffset = 0; colOffset < 8; colOffset++)
                {
                    if(abs(current[linha+lineOffset][coluna+colOffset]-reference[lineOffset+i][colOffset+j]) >= tolerance)
                        isEqual=0;
                }        
            }            

            //Se são iguais, salva a posição e o deslocamento do bloco
            if(isEqual){
                #pragma omp critical
                {
                    Ra[linha/8][coluna/8].l = linha-i;
                    Ra[linha/8][coluna/8].c = coluna-j;
                    Ra[linha/8][coluna/8].isBlock = 1;
                    Rv[linha/8][coluna/8].l = linha-i;
                    Rv[linha/8][coluna/8].c = coluna-j;
                    currentCorrespondence++;
                    //Atualiza a flag
                    for (int i = 0; i < 8; i++)
                    {
                        for (int  j = 0; j < 8; j++)
                        {
                            isBlock[linha+i][coluna+j]=1;
                        }
                    }
                    j+=8;                        
                }
            }
        }
        
    }    
}
