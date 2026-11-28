import redis

r = redis.Redis(host="broker", decode_responses=True)
N = 12
done = set()

for i in range(1, N + 1):
    r.lpush("tasks", i)

while len(done) < N:
    item = r.brpop("results", timeout=5)
    if item is None:
        for i in range(1, N + 1):
            if i not in done:
                r.lpush("tasks", i)
        continue
    _, result = item
    n = int(result.split("^")[0])
    if n not in done:
        done.add(n)
        print(result)
