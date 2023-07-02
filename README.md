# Servidor multi-usuário de chat em grupo

## Funcionamento do cliente
Após iniciarem a conexão, os usuários automaticamente enviam "REQ_ADD" para o servidor. Se forem aceitos, são tratados como clientes, e podem receber comandos via terminal.

Mensagens que podem ser recebidas do servidor:
- "RES_LIST([IdUseri], [IdUserj], …)". Quando essa mensagem for recebida, o cliente deve armazenar os valores dentro dos parênteses numa lista de usuarios disponíveis. Eles estão separados por ",". Ou seja, se o usuário recebe "RES_LIST(1, 2, 3)", ele deve armazenar os valores 1, 2 e 3 numa lista, sempre verificando se esse valor já existe.
- "MSG([IdSender], [IdReceiver], [Message])". Quando essa mensagem for recebida, deve ser impresso no terminal "[IdSender]: [Message]".
- "REQ_REM([IdUser])". Quando essa mensagem for recebida, [IdUser] deve ser retirado da lista de usuáros disponíveis.
- "ERROR([IdReceiver], [Code])". O cliente deve printar a mensagem de erro de acordo com o [code]. São elas:
	- 01: "User limit exceeded"
	- 02: "User not found"
	- 03: "Receiver not found"
- "OK([IdReceiver], [Code])". O cliente imprime na tela a mensagem baseada no código:
	- 01: "Removed Successfully"


Mensagens que o cliente pode enviar para o servidor:
- "REQ_ADD": pede para ser adicionado na rede. Após enviar essa mensagem, se receber um "ERROR(NULL, 1)", significa que não pode se desconectar ao servidor, ou seja, o cliente deve fechar o socket, e o programa é terminado. Se receber um valor inteiro, armazena esse valor como um inteiro [IdOrigin];
- "REQ_REM([IdOrigin])": pede para se desconectar do servidor. [IdOrigin] é o Id do usuário, armazenado quando o cliente se conectou ao servidor.
- "RES_LIST([IdOrigin])": pede uma lista dos IDs dos clientes do servidor;
- "MSG([IdOrigin], [IdReceiver], [Message])": envia a string [Message] para o cliente cujo ID é [IdReceiver];

Comandos que podem ser digitados nos terminais do cliente:
- "list users": envia a mensagem "RES_LIST[IdOrigin]" para o servidor;
- "send to [IdReceiver] "[Message]"": envia "MSG([IdOrigin], [IdReceiver], [Message])" para o servidor. [Message] está entre aspas duplas;
- "send all "[Message]"": envia "MSG([IdOrigin], NULL, [Message])" para o servidor. [Message] está entre aspas duplas;
- "close connection": envia "REQ_REM([IdOrigin])" para o servidor;

Para fins de debug, toda mensagem que o servidor receber é printada no terminal.



## Funcionamento do servidor
Os clientes enviam mensagens para o servidor, e o servidor serve como intermediário da rede.  
Mensagens que o servidor pode receber, e suas respostas:
- "REQ_ADD": o servidor verifica se o limite de clientes já foi batido. Se sim, envia "ERROR(NULL, 1)" para o usuário que enviou REQ_ADD. Se não, esse novo usuário recebe um ID, e é cadastrado no array clients[], e o servidor envia "MSG([NewUserId], NULL, "User [NewUserId] joned the group")" para todos os clientes da rede, onde [NewUserId] é o ID do novo usuário;
- "REQ_REM([IdSender])": o servidor deve checar se o usuário de ID [IdSender] está registrado rede (array clients[]). Se sim, envia a mensagem "REQ_REM([IdSender])" para todos os outros clientes da rede, com exceção do [IdSender]. Se não, envia "ERROR([IdSender], 2)" para o [IdSender];
- "RES_LIST([IdSender])": o servidor envia, para o [IdSender], a mensagem "RES_LIST([IdUseri], [IdUserj], …)", onde dentro do parênteses há todos os IDs conectados naquele momento, separados por ",";
- "MSG([IdSender], [IdReceiver], [Message])": se o cliente existir, o servidor envia essa mensagem para o cliente cujo ID é [IdReceiver], e envia um "OK([IdSender], 1)" para o [IdSender]. Se o cliente com o Id [IdReceiver] não existir, o servidor printa no próprio terminal "User [IdReceiver] not found", e envia para o cliente de ID [IdSender] a mensagem "ERROR([IdSender], 3)";
- "MSG([IdSender], NULL, [Message])": o servidor envia essa mensagem para todos os clientes da rede.
