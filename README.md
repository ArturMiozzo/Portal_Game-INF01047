
# Portal_Game-INF01047
Implementação de um jogo inspirado na série Portal com framework OpenGL em C++ 

## Contribuições dos membros:
- Mecânica dos portais: Artur Miozzo
- Testes de Colisões: Artur Miozzo
- Mapeamento de Texturas: Artur Miozzo
- Câmeras Look-At e Livre: Eric Cidade
- Iluminação e Shading: Eric Cidade
- Instânciamento de objetos: Artur e Eric
- Animações de movimento: Artur e Eric
- Curvas de Bezier: Artur e Eric
- Transformações geométricas: Artur e Eric

## Uso do chat GPT
O chat GPT foi consultado, porém foi de pouca usabilidade visto que ele utilizava de funções que não podiam ser utilizadas, ou muitas vezes respondia coisas sem sentido.
Caso a hitbox de um portal seja atingida e o outro tiver sido criado, a posição da camera é definida para logo a frente do portal, e o vetor view da camera é corrigido para a saida do portal.

## Processo de desenvolvimento
#### Mecânica dos portais
Os portais são criados projetando o vetor da camera em direção a algumas das paredes usando a hitbox de cada uma, assim que uma colisão é encontrada, o portal é gerado na posição X e Z, com Y fixo.
#### Testes de Colisões
Os testes de colisões foram utilizados para não permitir que a câmera saia da sala ou caia no buraco, que é um obstáculo do mapa. Além disso, os testes de colisões também foram utilizados para verificar se o disparo da portal gun acertou em alguma parede válida. Para isso, foram utilizados testes do tipo cubo-cubo e raio-cubo.
#### Mapeamento de Texturas
Todos os objetos tiveram suas cores calculadas a partir de imagens, ao todo, foram utilizadas 8 imagens distintas para as texturas, sendo elas:
- Chão
- Parede 1
- Parede 2
- Portal Gun
- Portal Azul
- Portal Laranja
- Cubo
- Botão
O mapeamento da Portal Gun, do Botão e do Cubo foram calculados no shader_vertex, enquanto a textura dos objetos restantes foi feito no shader_fragment.
#### Câmeras Look-At e Livre
A câmera livre serve como movimentação do personagem, enquanto a câmera look-at tranca na parede em movimento, como auxilio para mirar na parede.
#### Iluminação e Shading
#### Instânciamento de objetos
#### Animações de movimento
Tanto a animação da movimentação da câmera quanto a movimentação da parede foram calculadas a partir do tempo. Para a movimentação da câmera, foi utilizado o método de Euler, como visto em aula.
#### Curvas de Bezier
A curva de Bezier foi utilizada para a animação da parede em movimento.


## Manual
#### Teclas de movimentação:
 W - Andar para frente
 S - Andar para trás
 A - Andar para direita
 D - Andar para esquerda
 V - Noclip
#### Controle de Câmera:
 L - Alternar entre câmera look-at e livre
#### Interação com objetos:
E - Pegar/Soltar cubo
#### Controle da Portal Gun:
MB1: Atira portal azul
MB2: Atira portal laranja

## Passos necessários para compilação e execução
- Abrir o arquivo PortalGame.cbp no codeblocks
- Buildar o projeto e rodar
- É necessário ter instalada a biblioteca GLFW
