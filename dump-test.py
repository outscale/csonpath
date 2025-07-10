import csonpath

o = csonpath.CsonPath("$.a")
#breakpoint()
o = csonpath.CsonPath("$.a")
d =  {"B": "Bh", "a": "le a"}
r = o.find_first(d)

print(r)

o = csonpath.CsonPath("$.C")
r = o.find_first(d)

print(r)

o = csonpath.CsonPath("$[*]")
r = o.find_all(d)

print(r)

d = { "B": {"a": "true A"}, "a": "le a", "C": {"B": "not good", "a": "oh" } }
o = csonpath.CsonPath("$[*].a")
r = o.find_all(d)
print(r)

o.remove(o)
print(d)
#r["context"][0]['a'] = "a new one"
#print(r)
#print(d)
#del r["context"][0]['a']
#print(d)
