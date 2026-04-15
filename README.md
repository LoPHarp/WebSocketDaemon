# Налаштування автоматичного перезапуску (Systemd)

Для того щоб цей Daemon(демон) сам запускався при будь яких вилітах, треба створити системний сервіс.

1. Додати файл на сервері за шляхом: `/etc/systemd/system/websocket-daemon.service`
2. Записати у нього приблизно вміст (треба тестувати, шлях `ExecStart` потрібно замінити на реальний шлях до скомпільованого файлу):

```ini
[Unit]
Description=C++ WebSocket Push Daemon
After=network.target

[Service]
Type=simple
ExecStart=/absolute/path/to/build/WebSocketDaemon
Restart=always
RestartSec=5
User=root

[Install]
WantedBy=multi-user.target
```

3. Виконати команди для запуску та додавання в автозавантаження:
```bash
sudo systemctl daemon-reload
sudo systemctl enable websocket-daemon
sudo systemctl start websocket-daemon
```