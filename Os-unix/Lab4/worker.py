import socket
import time

import redis

r = redis.Redis(host="broker", decode_responses=True)
me = socket.gethostname()

while True:
    _, n = r.brpop("tasks")        # ждём задачу из очереди
    time.sleep(1)                  # имитация работы
    r.lpush("results", f"{n}^2 = {int(n) ** 2}  (выполнил {me})")
