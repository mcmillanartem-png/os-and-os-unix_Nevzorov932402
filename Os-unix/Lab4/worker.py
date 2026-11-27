import socket
import time

import redis

r = redis.Redis(host="broker", decode_responses=True)
me = socket.gethostname()

while True:
    n = r.blmove("tasks", "processing", 0, "RIGHT", "LEFT")  # задача не теряется
    time.sleep(1)                                             # имитация работы
    r.lpush("results", f"{n}^2 = {int(n) ** 2}  (выполнил {me})")
    r.lrem("processing", 1, n)                                # удаляем после записи результата
