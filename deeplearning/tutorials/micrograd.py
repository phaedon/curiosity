import math


class Value:
    def __init__(self, data, _children=(), _op="", label=""):
        self.data = data

        # Initialise the gradient of the node
        self.grad = 0.0

        # By default, this is just an empty function (i.e. a leaf node)
        self._backward = lambda: None

        self._prev = set(_children)
        self._op = _op
        self.label = label

    def __repr__(self):
        return f"Value(data={self.data})"

    def __add__(self, other):
        other = other if isinstance(other, Value) else Value(other)
        out = Value(self.data + other.data, (self, other), "+")

        def _backward():
            # Note: we use += here and everywhere else, because of the possibility that
            # we reuse nodes. The gradients therefore have to accumulate.
            # See multivariate case of the chain rule.
            self.grad += 1.0 * out.grad
            other.grad += 1.0 * out.grad

        out._backward = _backward
        return out

    def __neg__(self):
        return self * -1

    def __sub__(self, other):
        return self + (-other)

    def __mul__(self, other):
        other = other if isinstance(other, Value) else Value(other)
        out = Value(self.data * other.data, (self, other), "*")

        def _backward():
            self.grad += other.data * out.grad
            other.grad += self.data * out.grad

        out._backward = _backward
        return out

    def __rmul__(self, other):
        return self * other

    def __truediv__(self, other):
        return self * other ** (-1)

    def exp(self):
        x = self.data
        out = Value(math.exp(x), (self,), "exp")

        def _backward():
            # out.data because d/dx (e**x) == e**x
            self.grad += out.data * out.grad

        out._backward = _backward
        return out

    # "Division is a special case of something a bit more powerful"
    # a/b == a * b**(-1)
    # Therefore we'll implement exponentiation
    def __pow__(self, other):
        assert isinstance(
            other, (int, float)
        ), "only support int/float powers (for now)"
        out = Value(self.data**other, (self,), f"**{other}")

        def _backward():
            self.grad += (other * self.data ** (other - 1)) * out.grad

        out._backward = _backward
        return out

    # We could just implement exp() here in order to implement tanh
    # def exp(self): # etc
    # But there's no reason to implement all the component parts. As long as we know how to
    # create the local derivative of the "black box" function, that's all we need
    def tanh(self):
        x = self.data
        t = (math.exp(2 * x) - 1) / (math.exp(2 * x) + 1)
        out = Value(t, (self,), "tanh")

        def _backward():
            self.grad += (1 - t**2) * out.grad

        out._backward = _backward
        return out

    def backward(self):
        # First, build a topological graph:
        topo = []
        visited = set()

        def build_topo(v):
            if v not in visited:
                # This next line isn't strictly necessary, but helps when the Jupyter notebook
                # also contains some of the manual steps.
                v.grad = 0
                visited.add(v)
                for child in v._prev:
                    build_topo(child)
                topo.append(v)

        build_topo(self)

        self.grad = 1.0
        for node in reversed(topo):
            node._backward()
