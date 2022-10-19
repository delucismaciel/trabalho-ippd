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
#define tolerance 5

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

typedef struct {
    int l, c;
}Vector2;

typedef struct {
    Vector2 position;
    Vector2 offset;
}Offset;

//Primeiro frame, usado como referencia
unsigned char reference [video_height][video_width];
unsigned char current   [video_height][video_width];
//unsigned int qntByFrame [video_frames];

//Offset deslocamento[video_frames][1000];
Vector2 Ra[10000];
Vector2 Rv[10000];
int isBlock[video_height][video_width];
int currentCorrespondence = 0;

int fullFrameSize=video_width*video_height * 3 / 2;
int currentFrame = 0;

int main(int argc, char** argv){
    //Pega a referencia

    #pragma omp parallel for
    for (int i = 0; i < video_height; i++)
    {
        #pragma omp parallel for
        for (int j = 0; j < video_width; j++)
        {
            isBlock[i][j]=0;
        }
    }
    

    int selectedFrame = atoi(argv[1]);
    int compareFrame = atoi(argv[2]);
    printf("Frames Selecionados: %d e %d", selectedFrame, compareFrame);

    GetFrame(selectedFrame,1);
    OneFrame(compareFrame);

    printf("\n\tRa - posições no frame Atual");
    printf("\n\tRv - deslocamento entre o frame atual e a referencia");

    printf("\n----------------------\nTotal de correspondencias: %d\n",currentCorrespondence);

    for (int i = 0; i < currentCorrespondence; i++)
    {
        printf("\nRa[%d, %d] = Rv[%d, %d]",Ra[i].l,Ra[i].c,Rv[i].l,Rv[i].c);
    }
    return 0;
}

void OneFrame(int compareFrame){
    GetFrame(compareFrame,0);
    FrameVerifyByBlock(compareFrame);
}

void AllFrames(){
    
    #pragma omp parallel for private(currentFrame, current)
    for (int frame = 1; frame < video_frames; frame++)
    {
        //pega o frame atual
        GetFrame(frame);

        //verifica pixel à pixel
        FrameVerifyByBlock(frame);
        //PrintFrame(frame);


        //mostra a quantidade de achados por frame
        /*
        printf("\n\n\n ==============================================\n");
        printf("\n ==============================================\n\n\n");
        */
    }
}

void PrintFrame(int frame){
    for (int i = 0; i < video_height; i++)
    {
        for (int j = 0; j < video_width; j++)
        {
            printf("%d, ",(frame)?current[i][j]:reference[i][j]);
        }
        printf("\n");
    }
    
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

    #pragma omp parallel for
    for (int linha = 0; linha < video_height; linha+=8)
    {
        #pragma omp parallel for
        for (int coluna = 0; coluna < video_width; coluna+=8)
        {
            if(isBlock[linha][coluna])
                continue;
            Vector2 pos;
            //Compara apenas nos vizinhos mais próximos
            //ComparePixel(current[linha][coluna], linha, coluna, &pos);
            FindBlockNeighbor(linha, coluna);
            /*
            if(pos.l!=-1){
                //printf("\nVerificando [%d, %d](%d)  ==  [%d, %d]", linha, coluna,frame,pos.l,pos.c);
                    
                        //Verificar o bloco!
                        deslocamento[frame][qntByFrame[frame]].position = pos;
                        deslocamento[frame][qntByFrame[frame]].offset.l = linha-pos.l;
                        deslocamento[frame][qntByFrame[frame]].offset.c = coluna-pos.c;
                        *
                        #pragma omp critical
                        {
                            printf("\n---\nBloco encontrado! %d\n---\n", currentCorrespondence);
                            Rv[currentCorrespondence].l = linha-pos.l;
                            Rv[currentCorrespondence].c = coluna-pos.c;
                            Ra[currentCorrespondence].l = linha;
                            Ra[currentCorrespondence].c = coluna;
                            currentCorrespondence++;

                            for (int i = 0; i < 8; i++)
                            {
                                for (int  j = 0; j < 8; j++)
                                {
                                    isBlock[linha+i][coluna+j]=1;
                                }
                            }
                            
                            linha+=8;
                            coluna+=8;
                    }
                    
                
                    //qntByFrame[frame]++;
                    printf("\n\n\nAchou um bloco!\n");
                    printf("\tReference: [%d, %d] = %d",pos.l,pos.c,reference[pos.l][pos.c]);
                    printf("\n\tCurrent[%d]: [%d, %d] = %d",frame,linha,coluna,current[linha][coluna]);
                    printf("\n\tDeslocamento [%d, %d]\n\n\n",abs(linha-pos.l), abs(coluna-pos.c));
                
                }
            }
            */
        }    
    }    
}

void FindBlock(int linha, int coluna){
    #pragma omp parallel for schedule(dynamic,8)
    for (int i = 0; i < video_height; i++)
    {
        #pragma omp parallel for schedule(dynamic,8)
        for (int j = 0; j < video_width; j++)
        {
            if(abs(current[linha][coluna]-reference[i][j])<tolerance)
            {
                //Compara os Blocos
                if(Compare(i,j,linha,coluna)){
                    #pragma omp critical
                    {
                        //printf("\n---\nBloco encontrado! %d\n---\n", currentCorrespondence);
                        Rv[currentCorrespondence].l = linha-i;
                        Rv[currentCorrespondence].c = coluna-j;
                        Ra[currentCorrespondence].l = linha;
                        Ra[currentCorrespondence].c = coluna;
                        currentCorrespondence++;
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
}

void FindBlockNeighbor(int linha, int coluna){
    #pragma omp parallel for
    for (int i = MAX(0, linha-neighbor); i < MIN(video_height, linha + neighbor); i++)
    {
        #pragma omp parallel for
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
                    if(abs(current[linha+lineOffset][coluna+colOffset]-reference[lineOffset+i][colOffset+j])>=tolerance)
                        isEqual=0;
                }        
            }            

            //Se são iguais
            if(isEqual){
                #pragma omp critical
                {
                    //printf("\n---\nBloco encontrado! %d\n---\n", currentCorrespondence);
                    Rv[currentCorrespondence].l = linha-i;
                    Rv[currentCorrespondence].c = coluna-j;
                    Ra[currentCorrespondence].l = linha;
                    Ra[currentCorrespondence].c = coluna;
                    currentCorrespondence++;
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

int Compare(int rl, int rc, int cl, int cc){
    int isEqual=1;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if(abs(current[cl+i][cc+j]-reference[rl+i][rc+j])>=tolerance)
                isEqual=0;
        }        
    }
    return isEqual;    
}

void ComparePixel(unsigned char pixel, int linha, int coluna, Vector2 * out){
    Vector2 pos;
    pos.l=-1;
    pos.c=-1;

    for (int i = MAX(0,linha-neighbor); i < MIN(video_height,linha + neighbor); i++)
    {        
        if(pos.l!=-1)
            break;
        for (int j = MAX(0,coluna-neighbor); j < MIN(video_width,coluna + neighbor); j++)
        {
            //Se a diferença entre os pixels é menor que a tolerancia, achou uma posição para comparar.
            if(abs(reference[i][j]-pixel) <= 1){
                pos.l = i;
                pos.c = j;
                break;
            }
        }
    }
    *out = pos;   
}

int CompareBlock(Vector2 posR, int posCL, int posCC){
    int isEqual = 1;
    for (int l = 0; l < 8; l++)
    {
        for (int c = 0; c < 8; c++)
        {
            printf("\n\t* [%d, %d] (%d) == (%d) [%d, %d]\n",posR.l+l,posR.c+c,reference[posR.l+l][posR.c+c],current[posCL+l][posCC+c],posCL+l,posCC+c);
            if(abs(reference[posR.l+l][posR.c+c]-current[posCL+l][posCC+c]) > tolerance){
                isEqual=0;
                break;
            }
        }
        if(!isEqual)
            break;
    }
    return isEqual;
}