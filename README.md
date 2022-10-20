# Trabalho de IPPD

**Reconhecimento de blocos em frames de arquivos YUV usando OpenMPI**

Como executar (Com makefile):

make execute0 - Compila e executa a comparação entre o frame 0 e o frame 0. Como ambos são identicos, é esperado que o programa encontre os 3600 blocos do frame, sem ou com deslocamento.

make execute1 - Compila e executa a comparação entre o frame 0 e o frame 1.


1. Leitura do frame referência    - OK
2. Leitura do frame de comparação - OK
3. Comparação bloco a bloco       - OK
4. Print de resultados            - OK

Para executar(Sem makefile):
./main [frame1] [frame2] 

onde [frame1] e [frame2] são números inteiros, referentes ao número do frame

Printa na tela cada bloco, se foi achado ou não e seu respectivo descolamento caso encontrado.

Entrega o arquivo result.yuv com o resultado dos 2 frames

