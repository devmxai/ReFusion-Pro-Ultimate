# Platform implementations

This boundary owns native window, GPU, media, audio and lifecycle objects behind
Runtime contracts. Desktop GPU ownership begins with real Metal and D3D12
implementations. iOS and Android remain separate compile canaries until their
product gates; they may not redefine project or render semantics.
