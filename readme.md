# Servidor (Sistemas distribuídos)

_Este repositório consiste em um projeto de graduação desenvolvido para a cadeira de Sistemas distríbuidos_

**O repostiório `XXX` é uma aplicação cliente para o mesmo projeto**

Este projeto tem como intuíto básico servir como ponte de comunicação de dois tipos de clientes:

- Sensores
- Receptores

O servidor atuará como intermédio, recebendo informações de diversos sensores, e distribuindoos para os receptores...

## Protocolos

Para implemnetação seram utilizados 2 protocolos (próprios) a serem descritos abaixo

- `Protocolo de Transferência de Dados de Sensores`: **sdtp://**
- `Protocolo de Comunicação entre Clientes`: **ccp://**

### SDTP

A ideia deste protocolo é transferir de forma leve os dados adiquiridos pelos `Sensores`.

#### Conexão (Novo sensor)

O cliente deverá requisitar conexão ao servidor. Descrevendo seu nome e os tipos de dados que seram enviados, juntamente com sua identificação, seguindo a seguinte expressão regular: `\/[_a-zA-Z][_a-zA-Z0-9]*\/(([_a-zA-Z][_a-zA-Z0-9]*\[(str|num|bin)\])(&|$))+`

O servidor deverá então processar esses dados e atrelalos a uma estrutura capaz de armazenar as informações que o `Sensor` elencou. Após isso o deverá atrelar ao `Cliente` 2 identificadores distintos:

- Id público: Os outros outros clientes podem requisitar os dados do sensor por meio deste
- Id privado: O sensor ira atribuir seus dados por meio deste

O envio dos identificadores para o cliente deve seguir a seguinte expressão regular:
`\/priv\[[a-zA-Z0-9]+\]&pub\[[a-zA-Z0-9]+\]`

Exemplo:

```
Cliente  -- sdtp://192.168.0.1:4045/sensor_1/temp[num]&umid[str]&lun[bin] --> Servidor
Servidor -- sdtp://192.168.0.100:4030/priv[4de7s55de87]&pub[321s87de54s]  --> Cliente
```

#### Envio de informações

O envio deve ser feito afirmado o identificador (privado) e seguindo a ordem estabelecida na criação da conexão, com o valor podendo ser ignorado apenas não sendo colocando entre os `&&`, seguindo portanto a seguinte expressão regular: `\/[a-zA-Z0-9]\/([^&]*(&|$))+`

Exemplo:

```
Cliente -- stdp://192.168.0.1:4045/4de7s55de87/123.20&&0 --> Servidor
```

O servidor deve validar se o identificador corresponde ao servidor original e em caso positivo atribuir o novo dado a sua estrutura.

### Requisição de informação

1. Requisitando a lista de sensores

   Um cliente pode requisitar uma lista dos sensores disponíveis apenas enviado uma requisição sem dado nenhum.

   O servidor deverá responder a mesma informando o nome do sensor (informado na conexão) e o identificador público do mesmo, seguindo a seguinte expressão regular: `\/([_a-zA-Z][_a-zA-Z0-9]+\[[_a-zA-Z0-9]+\](&|$))+`

   Exemplo:

   ```
   Cliente  -- stdp://192.168.0.1:4045 --> Servidor
   Servidor -- stdp://192.168.0.150:4096/sensor_1[321s87de54s] --> Cliente
   ```

2. Requisitando os dados de um sensor

   Tendo o identificador público de um sensor o cliente pode requistar os dados de um ou mais sensores, seguindo a seguinte expressão regular: `\/([a-zA-Z0-9]+(&|$))+`

   O servidor deve responder a requisição seguindo a ordem da requisição dos , seguindo a seguinte expressão regular: `\/([^&]*(&|\](&|$)))+`

   Exemplo:

   ```
   Cliente -- stdp://192.168.0.1:4045/321s87de54s --> Servidor
   Servidor -- stdp://192.168.0.150:4096/[123.20&&0] --> Cliente
   ```
