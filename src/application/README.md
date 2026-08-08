# Application layer

Owns the process-local Application Host and the command-service boundary used by
Studio and CLI clients. The host is the only composition service that owns the
mutable Core authority; clients submit typed commands and consume copied accepted
snapshots/receipts.

Application also owns candidate admission: semantic validation, asset/font/
plugin capability resolution and Runtime render-program preparation complete
before one immutable accepted bundle is published. Studio, file watchers and a
GPU context may not perform a second post-accept activation.

Accepted publication has two no-fail phases. Application commits Core and
Runtime engine state while accepted-state readers are excluded, then releases
that lock before synchronously publishing immutable UI/diagnostic projections.
Projection code may read the accepted snapshot, but it may not mutate authority
or emit/reset Qt models from the engine-commit phase. A recursive mutex is not
an admitted substitute for this boundary.

Live Authoring coordination, transport policy, AssetDB orchestration, and other
engine-owned ports will be added behind this boundary in their scheduled gates.
Application may depend on Core; it must not depend on Qt or concrete platform
SDKs.
