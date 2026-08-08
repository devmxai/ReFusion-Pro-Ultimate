---
component: android-ndk
version: 28.2.13676358
status: contract-canary-pinned-not-runtime-qualified
owner: mobile-platform
official_origin: https://developer.android.com/ndk/downloads
license: Android SDK/NDK terms plus bundled third-party notices
---

# Android NDK intake

The G1 Android contract canary uses NDK `28.2.13676358` and API level 28 for
the `arm64-v8a` compile lane. CI obtains it through the official Android SDK
manager. The path is supplied through `ANDROID_NDK_HOME`; no host path is
committed and normal ReFusion configure/build performs no network access.

This intake qualifies only compiler/header/static-link closure for portable
Core, RFX5, RenderPlan, common Skia and fail-closed Vulkan adapter surfaces. It
does not qualify a device, Android UI, presentation, media, thermal behavior,
store packaging or runtime Vulkan support.
