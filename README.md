# Yell Peer-To-Peer Network Protocol

A simple peer-to-peer network protocol library using POSIX threads and sockets.

# Mountain Madness 2023 Submission

Whisper, a simple game created as an example of the yell peer-to-peer (p2p) network,
was created as a submission to the mountain madness 2023 hackathon.
This hackathon is organized by the <a href="https://sfucsss.org">CSSS</a> at <a href="https://sfu.ca">SFU</a>.

## Lost in Translation

The theme for this hackathon is 'lost in translation'.
There are several ways to interpret this theme,
and my game reflects several interpretations.
For example, it is a reflection on how data is
translated from clients to servers, or nodes to nodes, in networks.
If a program is written inefficiently, then this data is 'lost in translation'.
Thus, a strong and well written network protocol must be written to prevent this.
Additionally, the communication aspects of my game
permit users to become 'lost in translation'
through word-games, such as telephone, or others.

## Installation

It is recommended that players compile and use this software under a UNIX system.
`libyell` and `whisper` are compiled with the Clang compiler,
so this must be installed as well as dependencies.
To prepare `libyell` and compile `whisper`,
you must ensure that you have access to the `pthreads`
(POSIX multithreading) and `ncurses` development libraries.
The installation of these vary from platform to platform.
The `pthreads` library comes by default with Linux and MacOS.

Enter the following with respect to your operating system, as a super user (using `sudo`, if necessary):

Debian-based distributions (e.g., Ubuntu):  ```apt install clang libncurses-dev```

Arch-based distributions (e.g., Manjaro):   ```pacman -S clang ncurses```

MacOS, with <a href="https://brew.sh/">HomeBrew</a>:    ```brew install ncurses```  (MacOS' default C compiler is Clang.)

When these dependencies are installed, `cd` into `libyell`, and run:

```make clean libyell```

The static library file `libyell.a` will now be in `libyell/lib/libyell.a`.
From here, you must compile `whisper`.
To do this, `cd` into the repositories root directory, then `cd` into `whisper`.
Run:

```make clean whisper```

The binary file `whisper` will now be in `whisper/bin/whisper`.
Run the binary file `whisper` to play the game.

# Technology

Whisper showcases the capabilities of the yell p2p network,
allowing players to connect to the other player directly,
without using a server-client network model.
The benefits of this are that

1. The developer does not need to maintain server infrastructure.
2. It is freaking cool.

This project consists of two distinct parts:
the whisper game, and the yell library.

The yell library is a C implimentation of the ideas behind the yell p2p network.
This is compiled separately from the whisper game,
and statically linked as a library for use in the game.
Thus, the yell library is extensible beyond the scope of this project.
The yell library initiates a struct containing information about connected nodes,
and a thread that runs separate to another program's.
Helper functions are created that allow a program to
parse "events" received from other nodes in the network.
Events are occurences where messages were sent from one node to another.
Functions are also available for sending messages to all nodes, namely `yell`,
or for sending a message to a specific node, namely `yell_private`.

The whisper game utilizes the yell library to connect to and communicate with nodes.
Through this communication, textual messages are sent back and forth,
and also location information is transferred.
This game is created using the `ncurses` terminal-user-interface (TUI) library.
When opened, the player is prompted to choose a display name.
Then, they are situated as a ASCII person in an empty box.
The following controls are available to connect with and communicate with nodes:

* `c`: `c`onnect to another node. The player will be prompted to provide the domain and port of the node for which they are connecting.
* `y`: `y`ell to other nodes. The player will be prompted to enter the message they wish to send.
* `q`: `q`uit the game.
* Arrow-Keys: Move around the room. This movement will be synchronized across other nodes.
