## Сборка:
```
cd build && cmake .. && make && cd ../
```

## Запуск получателя:
```
./build/rdt_receiver 5005 received.txt
```

### Запуск отправителя:
```
./rdt_sender 127.0.0.1 5005 my_large_file.zip -d
```

## Примеры входных/выходные данных
### Обычный случай
I: `./rdt_receiver 5005 received.txt`

I: `./rdt_sender 127.0.0.1 5005 ../resources/tiny_file.zip -d`

O:
```
[DEBUG] Sending packet 0
[DEBUG] Sending packet 1
[DEBUG] Sending packet 2
[DEBUG] Sending packet 3
[DEBUG] Sending packet 4
[DEBUG] Received ACK 0
[DEBUG] Received ACK 1
[DEBUG] Received ACK 2
[DEBUG] Received ACK 3
[DEBUG] Received ACK 4

Transfer complete. Time: 0.00042688s, Speed: 11712.9 KB/s
```

### С потерями:
I: `./rdt_receiver 5005 received.txt`

I: `./rdt_sender 127.0.0.1 5005 ../resources/tiny_file.zip -d`

O:
```
[DEBUG] Sending packet 0
[DEBUG] Sending packet 1
[DEBUG] Sending packet 2
[DEBUG] Sending packet 3
[DEBUG] Sending packet 4
[DEBUG] Received ACK 0
[DEBUG] Received ACK 0
[DEBUG] Received ACK 0
[DEBUG] Received ACK 0
[DEBUG] Timeout! Resending window from 1
[DEBUG] Sending packet 1
[DEBUG] Sending packet 2
[DEBUG] Sending packet 3
[DEBUG] Sending packet 4
[DEBUG] Received ACK 4

Transfer complete. Time: 0.401781s, Speed: 12.4446 KB/s
```

## Описание реализации:
### 1. Формат пакета
Пакет передается как единая структура поверх UDP.
- `uint32_t seqNum`: Порядковый номер пакета (или ожидаемый ACK).
- `uint32_t type`: Тип пакета (0 - DATA, 1 - ACK).
- `uint32_t payloadSize`: Размер данных в байтах.
- `uint32_t checksum`: Простая сумма для проверки целостности.
- `char payload[1024]`: Полезная нагрузка (данные файла).

### 2. Выбранный алгоритм: Go-Back-N
*Отправитель*: Имеет "скользящее окно" размера N. Позволяет отправлять пакеты до тех пор, пока количество неподтвержденных пакетов не достигнет N. Используется один таймер для самого старого пакета в окне.
*Получатель*: Использует кумулятивные подтверждения. Ожидает пакет строго с номером `expectedSeqNum`. Если пакет верный - отправляет ACK и увеличивает счетчик. Если пришел пакет с другим номером - отбрасывает его и отправляет ACK для последнего успешно полученного пакета.

### 3. Конечный автомат
*Sender*:
- `IDLE`: Ожидание данных от файла. Если окно не полно - отправить пакет, запустить таймер.
- `WAIT_ACK`: Ожидание подтверждения через `select`().
- `TIMEOUT`: Если таймер истек, переотправить все пакеты в текущем окне и перезапустить таймер.
- `ACK_RECEIVED`: Сдвинуть окно (`base = ackNum + 1`). Если есть еще пакеты в окне - перезапустить таймер.
*Receiver*:
- `WAIT_DATA`: Ожидание UDP пакета.
- `CHECK`: Проверка контрольной суммы и `seqNum`. Если `seqNum == expectedSeqNum`, записать данные и отправить ACK. Иначе - повторить ACK для `expectedSeqNum - 1`.
