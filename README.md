# WinJection
This is a library for injecting packets into a live TCP connection, in both directions.

I originally coded it in Python, if you're interested you can take a look at the ![pyJection](https://github.com/Mou1z/pyJection) repository. 

It's important to note that whatever I'm aiming to achieve with the C++ version, can be achieved in Python as well. The primary motivation was processing speed and code security - python code can be decompiled while C++ makes it a bit harder. 

The main improvements I want to make in this are:
- Implement proper Multi-Threading and Packet Crafting

The structure of this class is very similar to the `PacketManager` from pyJection, you can read more about `pyJection` from ![here](https://github.com/Mou1z/pyJection).
