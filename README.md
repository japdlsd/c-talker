## UDP TALKER

This is my small project to System programming course. It's a console talker, which
enables two people to exchange messages between them via UDP protocol.
It's written in plain C with no extern libraries.


## INSTALLATION

Just run `make` in project's root directory.

## USAGE

`./talker.bin <IP address> [<port> <my port>]`

To send a message, just write some text and smash Enter :)
Note that while you're typing it's impossible to show newest income messages
(part of specification). They will be shown when you finish typing your message.

The only way to leave "typing mode" is to press Enter. If the resulting message
will be empty string, then no message will be sent (Note that two '\n' sybmols mean
string "\n", which isn't empty string).

To exit the program, press `Ctrl + C` (that will raise `SIGINT` signal).

Maximum length of a single message is defined in `MESSAGE_LENGTH` constant at
the beginning of source code. Right now it's set to 700. But program won't control it 
(it is hard due to using `select()`). If your message is longer, then it will be ferociously
cut into 700 bytes long chunks without proper EoL ('\n') symbols.
