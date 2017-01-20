EzClient.cpp
====


```cpp
auto client = ezclient::create("ws://localhost:9916");

client->on_worldinfo = [](ez::WorldInfo packet) {
	
};
client->on_joinplayer = [](ez::JoinPlayer packet) {
	std::cout<< "JoinPlayer : " << packet.player.player_id << std::endl;
};
client->on_leaveplayer = [](ez::LeavePlayer packet) {
	std::cout<< "LeavePlayer : " << packet.player.player_id << std::endl;
};
```

```cpp
while (true)
  client->dispatch();
```
