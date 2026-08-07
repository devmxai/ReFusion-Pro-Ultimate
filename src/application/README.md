# Application layer

Owns the process-local Application Host and the command-service boundary used by
Studio and CLI clients. The host is the only composition service that owns the
mutable Core authority; clients submit typed commands and consume copied accepted
snapshots/receipts.

Live Authoring coordination, transport policy, AssetDB orchestration, and other
engine-owned ports will be added behind this boundary in their scheduled gates.
Application may depend on Core; it must not depend on Qt or concrete platform
SDKs.
