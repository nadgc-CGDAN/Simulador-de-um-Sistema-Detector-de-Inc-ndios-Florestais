# Simulador-de-um-Sistema-Detector-de-Inc-ndios-Florestais
Como o sistema funciona?
O sistema de detector de incêndios florestal monitora e combate os incêndios no mapa florestal de dimensão 30x30. A floresta é composta por nós sensores que identificam os focos de incêndio em sua vizinhança e espalham as mensagens para os nós sensores vizinhos até que a central receba a mensagem e envie um bombeiro para apagar o foco de incêndio.

Os nós sensores estão agrupados em regiões específicas do mapa, geralmente em blocos 3x3, verificando a cada 1seg se houve alguma ocorrência de incêndio em sua vizinhança. A cada 5seg há um foco de incêndio na floresta e ao detectar um foco em suas vizinhanças, este nó sensor deve comunicar todos os nós sensores vizinhos para que estes espalhem a informação até o nó central que irá comunicar o bombeiro para apagar o fogo.

Cada nó é estruturado em uma thread, assim como a central e o bombeiro. Todo foco de incêndio é representado como @ registrado no arquivo incendios.log pela thread central a partir das informações recebidas pelos nós sensores. A thread bombeiro apaga os focos de incêndio a cada 2seg.

Implementação do código
O código foi dividido em 3 bibliotecas dinâmicas: sensores.c, utils.c e main.c. O arquivo sensores.c é responsável pela implementação das threads e da comunicação entre cada uma delas, o arquivo terá todo o escopo referente as estruturas das threads e das rotinas. O arquivo utils.c é usado como ferramenta para a construção e implementação dos outros códigos, ou seja, é um arquivo que conterá funções úteis e códigos repetitivos. Por fim, o arquivo principal que irá referenciar as rotinas do arquivo sensores.c e irá imprimir as interações com o mapa florestal.
