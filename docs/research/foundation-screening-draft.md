# ReFusion — مسودة الغربلة التأسيسية الأولى

> **الحالة: مسودة بحث ومراجعة — غير معيارية وغير معتمدة**
>
> هذه الوثيقة ليست مرجعًا للتطوير، ولا مواصفة تنفيذ، ولا Master Plan، ولا ADR، ولا تمنح إذنًا باختيار مكتبة أو تثبيت API أو كتابة بنية نهائية.
>
> الغرض الوحيد منها هو جمع ما نوقش، غربلة الاتجاهات والمرشحين، إظهار الفجوات والمخاطر، وتجهيز مادة قابلة للمراجعة تُستخدم لاحقًا في إعداد الـMaster Plan بعد اتخاذ القرارات واعتمادها صراحة.

- تاريخ المسودة: 2026-08-07
- الإصدار: Draft 0.8
- النطاق: الحقيقة الواحدة، محرك الأوامر، الرسم، الفيديو، الوسائط، الصوت، اللون، الـMotion Blur، الزمن، نموذج الطبقات، Studio Shell والـInspector، Live Authoring وAsset/Artifact Database، Agent/CLI/MCP/Diagnostics، تعديل ملفات المشروع خارجيًا، الوصول إلى منتج v1، والنشر والترقية والإضافات
- سلطة الوثيقة: استشارية فقط
- حالة القرارات: لا يوجد في هذه الوثيقة قرار تقني نهائي

---

## 1. هدف هذه الغربلة

تهدف هذه الجولة إلى الإجابة عن الأسئلة التالية، من دون القفز إلى التنفيذ:

1. ما المبادئ التي تبدو جوهرية لهوية ReFusion ولا يجوز أن تهدمها الخيارات التقنية؟
2. ما المسارات التقنية المنفصلة التي يجب ألا نخلط بينها؟
3. ما المرشحون الأقوى حاليًا لكل مسار؟
4. ما الادعاءات التي ما زالت تحتاج تجربة وقياسًا؟
5. ما الذي يمكن استبعاده مبدئيًا، وما سبب الاستبعاد؟
6. ما القرارات التي يجب حسمها قبل كتابة الـMaster Plan؟

هذه المسودة لا تجيب عن سؤال «كيف سنبني النظام بالكامل؟». بل تجهز الأدلة والأسئلة اللازمة كي تكون الإجابة اللاحقة مبنية على غربلة، لا على الانطباع الأول.

---

## 2. رموز الحالة المستخدمة

| الرمز | المعنى | هل يسمح بالتنفيذ؟ |
|---|---|---:|
| `P` | مبدأ منتج متفق عليه مبدئيًا، لكنه يحتاج صياغة معيارية لاحقًا | لا |
| `C` | مرشح ما زال داخل الغربلة | لا |
| `L` | اتجاه مرجح حاليًا، وليس قرارًا نهائيًا | لا |
| `E` | يحتاج تجربة تقنية أو قياسًا | لا |
| `D` | مؤجل من النطاق الحالي | لا |
| `X` | مستبعد مبدئيًا من دور محدد، ويمكن إعادة فتحه بدليل جديد | لا |

لا تعني كلمة «مرجح» أن الخيار أصبح معتمدًا. الاعتماد لا يحدث إلا في وثيقة قرار مستقلة بعد إتمام البحث والتجارب والمراجعة.

---

## 3. المبادئ الجوهرية التي ظهرت حتى الآن

### P-01 — مسار أوامر واحد

الاتجاه المتفق عليه مبدئيًا:

```text
UI Control ───────────────> UI Command Adapter ───────────────┐
                                                               │
External Agent ──> Atomic Project-File Edit ─> File Ingestor ──┤
                                                               ▼
                                                   ReFusion Command Engine
                                                               │
                                                        Validate + Apply
                                                               │
                                                               ▼
                                                    Accepted Project Revision
                                                               │
                                      ┌────────────────────────┼────────────────────────┐
                                      ▼                        ▼                        ▼
                                   Timeline                 Inspector                 Canvas
                                      │                        │                        │
                                      └────────────────────────┼────────────────────────┘
                                                               ▼
                                                      Unified Render Engine
```

المعنى المقصود حاليًا:

- لا تعدّل الواجهة حالة المشروع مباشرة.
- لا يوجد زر Agent مفترض داخل واجهة ReFusion؛ الأيجنت عميل خارجي شبيه بـCodex يعمل على ملفات المشروع.
- يكتب الأيجنت ملفات المشروع القابلة للعنونة مباشرة وفق Project Edit Protocol ذري، ثم يحوّل File Ingestor اللقطة المرشحة إلى semantic change خاضع للمحرك.
- لا يصبح التعديل الخارجي حقيقة تشغيلية لمجرد وصول filesystem event؛ يجب أن ينجح parse وschema migration وvalidation وatomic commit.
- UI Command والتعديل الخارجي يلتقيان عند المحرك نفسه، لا عند Widgets أو Canvas أو Timeline.
- الأمر المقبول ينتج Revision ذرّية واحدة.
- Timeline وInspector وCanvas تقرأ الـRevision المقبولة نفسها.
- Preview وExport يجب أن يستخدما الدلالة نفسها، حتى لو اختلف مستوى الجودة أو الجدولة.

ما لم يُحسم بعد:

- الصيغة الرسمية للـCommand Envelope.
- نموذج المعاملات والتعارضات.
- قواعد Undo/Redo وBranching وRecovery.
- حدود التعديلات المتزامنة.
- كيفية تمثيل الأخطاء والتحذيرات والـDiagnostics.
- هل كل التغييرات المشتقة تنتج Revision أم تبقى Cache خارج الحقيقة؟
- الصيغة النهائية لحزمة الملفات وcommit marker وآلية recovery عند تعديل أكثر من ملف.

### P-02 — حقيقة مشروع واحدة

الاتجاه الحالي أن الحالة الدائمة للمشروع يجب أن تكون واحدة، بينما تكون الملفات التالية مشتقة وقابلة لإعادة البناء:

- Thumbnails.
- Waveforms.
- Proxy media.
- Decode caches.
- GPU caches.
- Optical-flow fields.
- Diagnostics snapshots.
- Render previews.

ما لم يُحسم بعد:

- هل `project.refusion` ملف واحد أم حزمة ملفات ذات Manifest؟
- آلية الحفظ الذرّي والاسترداد بعد الانهيار.
- ترحيل Schema بين إصدارات المحرك.
- العلاقة الدقيقة بين `project.refusion` و`project.refusion.cpp`.
- سياسة Hashing وRelinking للأصول الخارجية.

### P-03 — الأصل الإعلامي لا يُعدّل أثناء التحرير

الـVideo Asset الأصلي يبقى immutable. تعديلات المستخدم والأيجنت تُخزّن كطبقات وخصائص وEffects وTime Mapping داخل المشروع. لا يحدث Baking أو تغيير للملف الأصلي إلا بأمر تصدير أو توليد أصل مشتق واضح.

### P-04 — الوقت الدلالي لا يعتمد على Pixels أو decimal seconds

الاتجاه الحالي استخدام وقت rational/ticks، مع ربط Timeline time إلى Source PTS بصورة صريحة. يجب دعم VFR وB-frames وKeyframe seeking وعدم افتراض:

```text
frame_number = seconds × fps
```

لم يُحسم بعد معدل ticks القياسي، وقواعد التقريب، وتمثيل time remapping، والمزامنة مع عينات الصوت.

### P-05 — لا توجد Capability وهمية

أي Layer أو Effect أو Property يستطيع الأيجنت استخدامها يجب أن تكون مسجلة في المحرك ولها على الأقل:

- هوية وإصدار.
- Schema للخصائص.
- Validation.
- Evaluation contract.
- Inspector representation.
- Render implementation أو تشخيص صريح بعدم التوفر.
- Platform support declaration.

---

## 4. فصل المسارات قبل مقارنة الأدوات

المناقشة الأولية أظهرت أن عبارة «المحرك الرسومي» تخفي عدة أنظمة مختلفة. لا يصح اختيار مكتبة واحدة ثم مطالبتها بأداء كل الأدوار.

| المسار | مسؤوليته | أمثلة المرشحين الحالية |
|---|---|---|
| Project/Command Engine | المشروع، الأوامر، التحقق، الـRevisions والوقت | Rust custom engine |
| 2D/Vector/Text Renderer | النصوص، العربية، الأشكال، المسارات والرسم الإجرائي | Skia، ومرشحون آخرون تحتاج مراجعة |
| GPU Render Graph | الـCompositing والـEffects والـCompute والموارد | wgpu، Dawn، Native APIs، بدائل أخرى |
| Studio UI | Timeline وInspector والنوافذ والمدخلات | Qt 6/QML، بدائل تحتاج مقارنة |
| Media Engine | Probe، demux، decode، seek، cache، proxy، encode | FFmpeg، Native APIs، Hybrid |
| Color Management | YUV conversion، working space، HDR، display/export transforms | OpenColorIO، custom pipeline، platform color APIs |
| Motion Analysis | Optical flow، velocity، occlusion | Custom GPU، مكتبات/نماذج مرشحة لم تُغربل بعد |
| 3D | Camera، lights، meshes، depth، PBR | مؤجل إلى غربلة مستقلة |

النتيجة المبدئية: لا توجد مقارنة صحيحة مباشرة بين Skia وGodot على أنهما يؤديان الدور نفسه. Skia مكتبة رسم، بينما Godot محرك تطبيق/ألعاب متكامل يفرض نموذج Scene وResources وRender loop.

---

## 5. غربلة مسار الرسم ثنائي الأبعاد والنصوص

### المتطلبات الأولية

- رسم متجهي عالي الجودة.
- Text shaping احترافي، وبالأخص العربية وRTL وfallback fonts.
- Paths، gradients، clips، masks وblend modes.
- دعم CPU fallback عند الحاجة.
- Offscreen rendering.
- Color-managed surfaces، بما فيها F16.
- إمكانية دمج Custom C++ drawing.
- عمل فعلي على macOS وWindows وiOS وAndroid.
- قابلية الاستخدام من Render Graph لا يملك حالة مشروع مستقلة.

### المرشح: Skia — `L + E`

نقاط القوة المرصودة:

- ناضج في 2D والنصوص والأشكال والمسارات.
- يوفر SkParagraph/SkShaper ويمكن دمجه مع HarfBuzz وICU.
- يوفر CPU وGPU paths ويدعم offscreen surfaces.
- مناسب لفكرة `Custom Skia Layer` وC++20 SDK.
- ترخيص BSD مرن.
- SkSL Runtime Effects مفيدة لبعض المؤثرات المكانية.

المخاطر والأسئلة:

- اختيار Ganesh أم Graphite، وما درجة نضج كل backend على منصات ReFusion؟
- كيف ستتم مشاركة textures والمزامنة مع GPU Render Graph؟
- هل ستكون Skia مالك جهاز الـGPU أم مستهلكًا لموارد يملكها ReFusion؟
- حجم البناء، cadence التحديث، وربط الاعتماد في CI.
- ضرورة اختبار العربية المركبة، variable fonts، fallback، vertical metrics وemoji.

الحكم المؤقت:

> Skia أقوى مرشح حاليًا لمسار 2D/Text، لكنها غير معتمدة حتى نجاح تجارب النص، اللون، والـGPU interop.

### المرشح: Godot كمحرك ReFusion الأساسي — `X` للدور الأساسي

سبب الاستبعاد المبدئي من دور «المحرك الأساسي»:

- يجلب Scene Tree وResources وRender loop كنموذج حالة إضافي.
- يزيد خطر وجود حقيقة موازية لحقيقة Project Revision.
- RenderingServer مصمم ضمن افتراضات Godot، لا ضمن Command/Revision architecture الخاصة بـReFusion.
- استخدامه كأساس يعني تكييف ReFusion مع محرك ألعاب متكامل بدل امتلاك ReFusion لمحركه الدلالي.

ما لا يعنيه الاستبعاد:

- لا يعني أن Godot ضعيف تقنيًا.
- لا يمنع دراسة أجزاء أو أفكار أو استخدام مستقل في Prototype غير منتج.
- يمكن إعادة فتحه فقط إذا أثبتت تجربة أنه يعمل كمكوّن renderer بلا حالة موازية وبكلفة أقل بوضوح.

### مرشحون لم يُبحثوا بما يكفي — `C`

- Qt painting/scene graph facilities.
- Cairo.
- Blend2D.
- مكتبات vector/text أخرى.

لا توجد بيانات كافية حاليًا لتقييمها أو استبعادها. يجب ألا تُمنح Skia الفوز النهائي قبل إجراء مقارنة مركزة في المتطلبات التي تهم ReFusion فعلًا.

---

## 6. غربلة GPU Render Graph

### المتطلبات الأولية

- Metal على Apple، D3D12 على Windows، Vulkan على Android.
- WGSL أو shader model قابل للتوحيد والتحقق.
- Render passes وCompute passes.
- Resource lifetime وtransient textures.
- Fences، barriers، queues وexternal texture interop.
- HDR وRGBA16F على الأقل.
- حتمية دلالية بين Preview وExport.
- تشغيل headless/offscreen للتصدير والاختبارات.

### wgpu/WGSL — `L + E`

نقاط القوة المرصودة:

- ينسجم مع قلب Rust.
- يوفر backends متعددة تحت API واحدة.
- WGSL مناسب لفكرة Custom Effect مسجلة وقابلة للتحقق.
- مناسب للـCompute والـCompositing وإدارة passes.

المخاطر والأسئلة:

- استيراد textures الخارجة من VideoToolbox وMedia Foundation وMediaCodec.
- مشاركة الجهاز والـqueues مع Skia وQt.
- الفروق بين المنصات والميزات المتاحة فعليًا.
- تكلفة الوصول إلى native handles أو APIs منخفضة المستوى عند الحاجة.
- سياسة ثبات API وإصدارات الاعتماد.

### Dawn/WebGPU native — `C + E`

يجب مقارنته مع wgpu، خصوصًا لأن Skia تستخدم Dawn في بعض مسارات البناء. الأسئلة المركزية:

- هل يقلل Dawn كلفة تكامل Skia مقارنة بـwgpu؟
- هل يضعف اختيار Dawn تكامل Rust أو يزيد كلفة C ABI؟
- ما حالة external memory وshared textures لكل منصة؟

### Native Metal/D3D12/Vulkan مباشرة — `C + E`

نقاط القوة المحتملة:

- أقصى تحكم في interop والأداء والـvideo surfaces.
- وصول مباشر إلى خصائص المنصة.

الكلفة المحتملة:

- ثلاثة render backends فعلية واختبارات وصيانة منفصلة.
- خطر اختلاف السلوك البصري بين المنصات.
- كلفة عالية على فريق صغير أو في بداية المشروع.

### Qt QRhi كمحرك ReFusion الرسومي — `X` مبدئيًا للدور الأساسي

سبب الاستبعاد المبدئي: QRhi API خاصة في Qt ولا تقدم ضمانات source/binary compatibility. قد تبقى مفيدة داخل تكامل الواجهة، لكنها ليست مرشحًا جيدًا حتى الآن لتكون العقدة الأساسية لمحرك ReFusion طويل العمر.

### الحكم المؤقت

> wgpu هو الاتجاه المرجح حاليًا، لكن أكبر خطر معماري في المشروع كله هو GPU interop بين Skia وdecoder وQt وwgpu. لا يجوز اعتماده قبل تجربة متعددة المنصات تثبت ownership والمزامنة وعدم وجود readback إجباري.

---

## 7. غربلة واجهة الاستوديو

### Qt 6/QML — `L + E`

الأسباب المبدئية:

- مناسب لتطبيقات سطح المكتب المعقدة والنوافذ والـinput والـaccessibility.
- يتيح بناء Timeline وInspector وPanels من دون جعل UI مصدر حقيقة.
- قابل للفصل عن المحرك بواسطة C ABI أو طبقة ربط محددة.

أسئلة لم تُحسم:

- جودة وتجربة Qt على iOS وAndroid بالنسبة لمنتج إبداعي ثقيل.
- دمج Canvas texture بكفاءة مع render backend المختار.
- سياسة الترخيص والتوزيع والبناء.
- هل ستكون واجهة الهاتف هي Studio كاملة أم تجربة مختلفة مبنية على المحرك نفسه؟

الحكم المؤقت:

> Qt/QML مرشح قوي للـStudio UI، وليس Renderer المشروع ولا مصدر حالة المشروع.

---

## 8. غربلة Media Pipeline

### القاعدة المفاهيمية الحالية — `P`

Skia لا تفك ترميز الفيديو. Media Engine تسلم Render Graph إطارًا موصوفًا بالتوقيت واللون والتخزين، ثم يرسم Skia أو الـGPU compositor فوقه أو معه.

```text
Immutable Media Asset
        │
        ▼
Probe + Demux + Index
        │
        ▼
Compressed Packets
        │
        ▼
Hardware/Software Decoder
        │
        ▼
DecodedVideoFrame
PTS + Duration + YUV Planes + Color Metadata
        │
        ▼
GPU Color Conversion / Render Graph
        │
        ▼
Skia Layers + Effects + Composite
```

### الخيار A: FFmpeg لكل شيء — `C`

نقاط القوة:

- تغطية واسعة للحاويات والصيغ.
- libavformat قوي للـprobe/demux/timestamps/seeking.
- libavcodec يوفر software fallback وhardware contexts متعددة.
- API موحدة نسبيًا عبر المنصات.

المخاطر:

- zero-copy ليس مضمونًا تلقائيًا لكل backend.
- بعض مسارات hardware acceleration قد تفرض نسخًا أو قيود surface pools.
- تعقيدات LGPL/GPL والمكتبات الخارجية والتوزيع على متاجر المنصات.
- ربط Project/Engine types مباشرة بـAVFrame سيخلق اعتمادًا عميقًا يصعب تغييره.

### الخيار B: Native Media APIs لكل منصة — `C`

المسار المحتمل:

| المنصة | Decoder/Encoder | GPU frame representation |
|---|---|---|
| macOS/iOS | VideoToolbox | CVPixelBuffer/IOSurface/Metal texture |
| Windows | Media Foundation + D3D video | DXGI/D3D texture |
| Android | MediaCodec | Surface/AHardwareBuffer/Vulkan image |

نقاط القوة:

- أفضل تكامل محتمل مع hardware decode/encode والمنصة.
- تحكم أفضل في surface pools وzero-copy.

المخاطر:

- ثلاث أو أربع implementations مستقلة.
- اختلاف دعم الحاويات والصيغ والسلوك.
- كلفة أعلى للاختبار والصيانة ومعالجة الحالات الطرفية.

### الخيار C: Hybrid Media Engine — `L + E`

الاتجاه المرجح حاليًا:

- واجهة ReFusion Media مستقلة لا تكشف `AVFrame` أو platform handles للمشروع.
- FFmpeg/libavformat للـprobe والديمكس والفهرسة والتوافق الواسع.
- libavcodec software fallback.
- Hardware adapters أصلية أو FFmpeg hwcontext حسب ما تثبته تجربة كل منصة.
- `DecodedVideoFrame` داخلي يستطيع حمل Native GPU Handle مضبوط الملكية.

سبب الترجيح:

- يحافظ على تغطية الصيغ مع إبقاء باب zero-copy مفتوحًا.
- يمنع تحول FFmpeg أو API منصة إلى حقيقة معمارية للمشروع.
- يسمح باستبدال backend من دون تغيير Commands أو Project Document.

المخاطر:

- أعلى تعقيد تصميمي في حدود الـABI والـlifetime والمزامنة.
- قد ينتهي بنا الأمر إلى صيانة مسارين لكل منصة إذا لم نضع سياسة fallback واضحة.

الحكم المؤقت:

> Hybrid هو الاتجاه الأقوى حاليًا، لكن لا اعتماد قبل قياس decode/seek/interop على الأجهزة المستهدفة ومراجعة التوزيع والترخيص.

---

## 9. شكل داخلي مبدئي لإطار الفيديو — للاستكشاف فقط

هذا الشكل ليس API معتمدًا، بل يوضح المعلومات التي لا يجوز فقدها بين decoder وrenderer:

```text
DecodedVideoFrame
├── source_asset_id
├── source_pts: RationalTime
├── duration: RationalTime
├── coded_size
├── display_size
├── pixel_aspect_ratio
├── orientation
├── pixel_format
├── bit_depth
├── color_primaries
├── transfer_function
├── matrix_coefficients
├── color_range
├── hdr_metadata
├── alpha_mode
└── storage
    ├── Metal/CVPixelBuffer
    ├── D3D texture
    ├── AHardwareBuffer/Vulkan image
    └── CPU planes fallback
```

الأسئلة الحرجة:

- من يملك surface ومتى يعاد إلى decoder pool؟
- كيف ينتقل fence/semaphore بين decoder وrenderer؟
- هل يتم YUV→RGB في backend الأصلي أم shader موحد؟
- هل يمكن إبقاء الإطار GPU-resident حتى encoder؟
- كيف تُدار frames ذات multiple planes و10/12-bit؟

---

## 10. غربلة إدارة اللون وHDR

### المتطلبات الأولية

- عدم فرض sRGB/8-bit على كل المصادر.
- حفظ primaries وtransfer وmatrix وrange وHDR metadata.
- YUV→RGB مضبوط وقابل للاختبار.
- Working space خطية للـcompositing.
- دعم SDR وHDR وwide gamut.
- Display transform منفصل عن export transform.
- تطابق اللون بين Preview وExport ضمن حدود قياس معتمدة لاحقًا.

### OpenColorIO — `L + E`

سبب الترجيح:

- موجه للإنتاج السينمائي والمؤثرات.
- يدعم ACES ومسارات تحويل قابلة للتكوين.
- يقلل اختراع نظام color management خاص من الصفر.

الأسئلة:

- كلفة التكامل على الموبايل وحجم البناء.
- توليد shaders وتكاملها مع backend المختار.
- الحدود بين OCIO وبين platform display/HDR APIs.
- ما الـworking space الافتراضية لـReFusion؟ لا قرار حتى الآن.

### نظام مخصص بالكامل — `C`

قد يكون ضروريًا لمرحلة YUV conversion أو عمليات محددة، لكنه يحمل خطر إعادة بناء نظام ألوان سينمائي كامل وصعوبة التحقق منه.

الحكم المؤقت:

> OCIO مرشح قوي لإدارة التحويلات الإبداعية والعرض، مع احتمال بقاء YUV decode transforms والمنصة ضمن طبقات متخصصة. الحدود الدقيقة تحتاج Prototype.

---

## 11. غربلة Motion Blur

### التمييز المطلوب

هناك أربعة أشياء مختلفة لا ينبغي تسميتها Effect واحدة بلا توضيح:

| النوع | الوظيفة | الحالة |
|---|---|---|
| Gaussian Blur | ضبابية مكانية بلا زمن | `X` كبديل للموشن بلور الاحترافي |
| Directional Blur | sampling مكاني في اتجاه ثابت | `C` كEffect مستقل، لا كحل كامل |
| Transform Motion Blur | عدة عينات زمنية لحركة Layer/Camera | `L + E` |
| Pixel Motion Blur | Optical flow وper-pixel velocity | `L + E` للجودة العالية |

توثيق Skia يوضح أن `SkImageFilters::Blur` يستخدم sigma على X وY؛ لذلك لا يمثل تعريض كاميرا زمنيًا بذاته.

### Transform Motion Blur — الاتجاه المبدئي

عند output time محدد:

```text
exposure = frame_duration × shutter_angle / 360
```

يقيّم المحرك Layer subtree عند عدة أزمنة ضمن shutter interval، وترسم Skia المحتوى عند كل sample، ثم يجمع GPU Render Graph العينات بأوزان مضبوطة في linear premultiplied HDR surface.

أسئلة تحتاج قرارًا وتجربة:

- shutter phase convention.
- sample distribution والـweighting function.
- adaptive sampling.
- ترتيب masks/effects/transform/blend بالنسبة للـblur.
- التعامل مع frame boundaries وtime remapping.
- جودة Preview مقابل Export مع الحفاظ على الدلالة نفسها.

### Pixel Motion Blur — الاتجاه المبدئي

يحتاج:

- Forward/backward optical flow.
- Confidence map.
- Occlusion/disocclusion handling.
- Per-pixel velocity buffer.
- GPU warping/accumulation.

المخاطر:

- ghosting وedge tearing.
- فشل flow عند القطع، الوميض، الشفافية، الضوضاء والحركة السريعة.
- كلفة GPU والذاكرة.
- الحاجة إلى cache مشتق مرتبط بالـRevision والمصدر والجودة.

### Hybrid Motion Blur — `L + E`

الاتجاه المرغوب للجودة العليا:

```text
Final velocity =
    layer transform velocity
  + camera velocity
  + per-pixel optical-flow velocity
```

لكن هذا لا يُعتمد قبل مقارنة خوارزميات Optical Flow واختبار occlusion والأداء على المنصات المستهدفة.

### دور Skia في Motion Blur

Skia مرشحة لرسم كل عينة ومحتوى الـLayer والـmasks والـvectors. أما جدولة الزمن وطلب video frames وحساب flow وتجميع العينات فهي مسؤولية ReFusion Render Graph. يمكن استخدام SkSL لمؤثرات مكانية أو sampling محدود، لكنه ليس بديلًا عن النظام الزمني.

---

## 12. مصفوفة غربلة أولية

الدرجات التالية فرضيات بحثية من `1` إلى `5`، وليست قياسات أو قرارًا. الغرض كشف مواضع الجهل التي تحتاج تجارب.

### الرسم والمحرك

| المرشح/التركيب | ملاءمة الحقيقة الواحدة | 2D/Text | GPU Effects | Cross-platform | كلفة التكامل | مستوى الدليل |
|---|---:|---:|---:|---:|---:|---|
| Custom ReFusion + Skia | 5 | 5 | 3 | 4 | 2 | متوسط |
| Custom ReFusion + Skia + wgpu | 5 | 5 | 5 | 4 | 1 | منخفض حتى اختبار interop |
| Godot كأساس كامل | 2 | 3 | 4 | 4 | 3 ظاهريًا، وقد تصبح 1 مع التخصيص | متوسط |
| Qt بوصفه UI فقط | 5 | غير منطبق | غير منطبق | 4 | 3 | متوسط |

### الوسائط

| المرشح | تغطية الصيغ | Zero-copy محتمل | توحيد المنصات | كلفة الصيانة | مرونة الاستبدال | مستوى الدليل |
|---|---:|---:|---:|---:|---:|---|
| FFmpeg فقط | 5 | 3 | 5 | 4 | 2 إذا تسربت أنواعه | متوسط |
| Native فقط | 3 | 5 | 1 | 1 | 3 | متوسط |
| Hybrid خلف ReFusion API | 5 | 5 | 4 | 2 | 5 | منخفض حتى الـPrototype |

لا يجوز استخدام هذه الدرجات كتبرير اعتماد. يجب استبدالها بنتائج وتجارب وروابط بحث في جولة لاحقة.

---

## 13. التجارب المطلوبة قبل القرارات

### E-001 — Skia Text/Arabic Quality

اختبار:

- Arabic shaping وRTL.
- mixed Arabic/Latin.
- ligatures وdiacritics.
- fallback fonts وemoji.
- variable fonts.
- metrics متطابقة في Preview وoffscreen export.

بوابة النجاح: تحدد لاحقًا بأمثلة مرجعية واختبارات صور golden، لا بالمشاهدة الانطباعية فقط.

### E-002 — GPU Ownership and Interop

إنشاء جهاز GPU واحد أو عقدة ownership واضحة، ثم:

- render Skia إلى texture.
- تمريرها إلى compositor بلا CPU readback.
- مزامنة صحيحة بواسطة fences/semaphores.
- إعادة استخدامها في Qt Canvas.

يُنفذ على Metal وD3D وVulkan، لا على منصة واحدة فقط.

### E-003 — Hardware Video Decode to Composite

لكل منصة:

```text
4K 10-bit source
→ hardware decode
→ native YUV GPU surface
→ color conversion
→ Skia Arabic overlay
→ composite
→ display
```

القياسات المطلوبة:

- عدد نسخ الإطار.
- latency.
- sustained playback FPS.
- CPU/GPU utilization.
- memory and surface-pool pressure.
- seek latency.
- color correctness.

### E-004 — Time and Seek Correctness

مجموعة مصادر تشمل:

- CFR وVFR.
- B-frames.
- long GOP.
- non-zero start PTS.
- rotation وpixel aspect ratio.
- audio/video drift cases.

يجب إثبات أن طلب Project Tick محدد يعيد الإطار الصحيح وفق PTS، لا وفق رقم فريم مفترض.

### E-005 — Preview/Export Semantic Parity

نفس Project Revision ونفس tick يجب أن ينتجا المعنى البصري نفسه. يسمح للـPreview بتقليل samples أو resolution فقط عندما يكون ذلك معلنًا كQuality policy، لا كمسار Effects مختلف.

### E-006 — Transform Motion Blur

مقارنة:

- 4، 8، 16، 32 sample.
- box/triangle/بدائل weighting.
- shutter angle وphase.
- الحركة الخطية والدوران والـscale والكاميرا.
- alpha edges وmasks.

### E-007 — Optical Flow and Pixel Motion Blur

يجب أولًا غربلة المرشحين للخوارزمية، ثم اختبار:

- fast motion.
- occlusion/disocclusion.
- scene cuts.
- thin geometry.
- noisy/low-light footage.
- transparency.
- 4K performance على desktop والموبايل.

### E-008 — Packaging and License Audit

فحص فعلي لكل dependency وبناء:

- Skia وملحقاتها.
- FFmpeg configure flags والمكتبات الخارجية.
- Qt licensing model.
- OpenColorIO.
- shader/compiler dependencies.
- متطلبات App Store والمنصات.

هذه مراجعة هندسية/توزيعية وليست استشارة قانونية.

---

## 14. المخاطر الكبرى التي ظهرت

### R-01 — تعدد مالكي جهاز GPU

إذا أنشأ Qt وSkia وwgpu والـdecoder أجهزة أو queues مستقلة بلا عقد واضح، قد يحدث:

- texture copies.
- stalls.
- synchronization bugs.
- memory duplication.
- اختلاف backend بين Canvas وExport.

هذا الخطر يجب حله قبل توسيع نظام الـEffects.

### R-02 — تسرب تفاصيل المكتبات إلى Project Model

ظهور `AVFrame` أو `SkImage` أو `wgpu::Texture` أو platform handle داخل Project Document سيخلط الحقيقة الدائمة بالموارد المؤقتة ويصعّب الاستبدال والترحيل.

### R-03 — الخلط بين جودة الـPreview ودلالة الرندر

يجوز تقليل الجودة في Preview، لكن لا يجوز أن يصبح لها Effect semantics مختلفة عن Export من دون إعلان واختبار.

### R-04 — توسيع النطاق مبكرًا

3D والسوائل والمحاكاة وParticles وAI optical flow عالي الجودة مجالات كبيرة. إدخالها كلها في النواة الأولى قد يمنع إثبات المسار الأساسي:

```text
Command → Revision → Evaluate → Render → Export
```

### R-05 — اللون كإضافة متأخرة

إذا صُممت textures والـeffects على افتراض 8-bit sRGB ثم أضيف HDR لاحقًا، ستكون كلفة التصحيح كبيرة. يجب تحديد color/alpha contracts قبل تثبيت Render Graph.

---

## 15. الأمور المؤجلة من هذه الجولة

هذه المسودة لا تغربل نهائيًا:

- محرك 3D.
- Audio engine والتأثيرات الصوتية.
- Collaboration/network synchronization.
- AI model execution.
- Plugin sandboxing.
- Scripting/expressions language.
- Project serialization format.
- Export codec/product licensing strategy.
- Cloud rendering.

يجب فتح مسودة غربلة مستقلة لكل مسار عندما يحين دوره.

---

## 16. الصورة المعمارية المرجحة حاليًا — غير معتمدة

> هذا الرسم يلخص اتجاه البحث فقط، ويحظر اعتباره مخطط تنفيذ.

```text
UI Commands ──────────────────────────────────────────────┐
                                                         │
Agent Project-File Edits ─> Live Authoring/File Ingestor ─┤
                                                         ▼
                                            Rust Command/Revision Engine
                                                         │
                                                         ▼
                                               Project Frame Evaluator
                                                         │
              ┌───────────────────┼───────────────────┐
              ▼                   ▼                   ▼
        Media Engine       Skia 2D/Text        Effect Graph
      FFmpeg + Native         Candidate       wgpu/WGSL Candidate
              │                   │                   │
              └───────────────────┼───────────────────┘
                                  ▼
                        Unified GPU Frame Graph
                                  │
                     ┌────────────┴────────────┐
                     ▼                         ▼
                  Preview                    Export
```

مواضع عدم اليقين الرئيسية في الرسم:

- هل wgpu هو backend الصحيح أم Dawn أو native APIs؟
- من يملك GPU device؟
- كيف يتكامل Skia مع الجهاز والـtextures؟
- ما backend الوسائط الفعلي لكل منصة؟
- أين تنتهي مسؤولية OCIO وتبدأ مسؤولية shaders والمنصة؟

---

## 17. بوابة الانتقال إلى الـMaster Plan

يجب التفريق بين نوعين من الأدلة:

1. **Decision/Kill-Risk Spikes قبل الخطة:** تجارب قصيرة قد تثبت أن اتجاهًا أساسيًا مستحيل أو مكلف على نحو يغير المعمارية.
2. **Delivery/Conformance Gates داخل الخطة:** اختبارات بناء وإنتاج واعتمادية لا يمكن منطقيًا إكمالها قبل وجود مراحل تنفيذ.

اشتراط إنهاء جميع تجارب الإثبات والتنفيذ قبل كتابة Master Plan سيؤدي إلى تنفيذ مشروع بلا خطة، وهو عكس الغرض. لذلك تصبح بوابة كتابة الخطة مبدئيًا:

1. مراجعة هذه الغربلة واعتماد Product Contract وNot‑V1 صريحين.
2. تثبيت Creator Loop واحدة مستهدفة للإصدار المدفوع الأول.
3. تحديد القرارات التي قد تقتل المشروع وإجراء spikes محدودة لها: GPU ownership/interoperability، hardware decode surface path، unified layer/property prototype، preview/export path، وsigned desktop packaging skeleton.
4. تحديد ownership واضح للـGPU والموارد والمزامنة والزمن.
5. تحديد الحدود الداخلية لـProject/Command/Media/Audio/Render/Plugin من دون تسريب أنواع الطرف الثالث.
6. صياغة color/alpha/time/audio/channel contracts أولية قابلة للاختبار.
7. مراجعة قانونية/ترخيصية للتوزيع التجاري، خصوصًا Qt وFFmpeg/codecs ومكتبات الطرف الثالث.
8. تحويل القرارات المختارة إلى ADRs مستقلة أو وضع Decision Deadline واضح لها داخل أول Gate.
9. وضع كل التجارب E-001 إلى E-072 داخل milestone وExit Criteria محددين؛ لا تبقى قائمة منفصلة بلا مالك.
10. مراجعة بشرية صريحة تؤكد أن نطاق الخطة قابل للتمويل والتنفيذ والإصدار.

بعد ذلك يُكتب Master Plan يحدد المعمارية والمراحل والـAPIs والاختبارات والتنفيذ. ولا تبدأ مرحلة تالية فيه حتى تمر بوابة السابقة بأدلة محفوظة.

---

## 18. أسئلة الجولة التالية

1. هل نبدأ بحسم GPU ownership وSkia interop لأنه أعلى خطر؟
2. هل نحدد أولًا نطاق المنصات للنسخة الرأسية الأولى: macOS فقط للاختبار أم desktop مزدوج منذ البداية؟
3. ما الحد الأدنى المهني المطلوب من الفيديو في أول Prototype: 1080p أم 4K، SDR أم HDR، H.264 فقط أم HEVC أيضًا؟
4. هل iOS وAndroid هدف إصدار أول أم هدف معماري يُختبر مبكرًا ويُشحن لاحقًا؟
5. هل Pixel Motion Blur جزء من النواة الأولى أم Phase لاحقة بعد Transform Motion Blur؟
6. هل `project.refusion.cpp` يُحمّل داخل العملية أم يُبنى ويشغّل في sandbox منفصل؟
7. هل Preserve multichannel هي سياسة الإدراج الافتراضية، مع Split Channels كخيار صريح؟
8. ما الحد الأدنى لأول Audio Studio: Clip restoration فقط، أم Clip + Track Mixer + Master منذ البداية؟
9. هل استضافة VST3/AU/AUv3 تدخل الإصدار الأول أم تؤجل بعد استقرار ReFusion Audio Graph؟
10. هل نقبل جعل Windows 11 Version 25H2 حدًا أدنى لمسار Media Foundation hardware-only الصارم؟
11. هل الملف غير المدعوم عتاديًا يُرفض عند الإدراج أم يُدرج كـOffline Asset مع Diagnostic؟
12. هل Canvas تستخدم native viewport يملكها المحرك أم texture native مغلفة داخل Qt Quick؟
13. هل الأصل ذو PTS ناقص أو متناقض يُرفض أم يُدرج كـTiming-Offline في Strict Timing Mode؟
14. هل يوجد Compatibility Mode منفصل لإصلاح التوقيت بالتقدير، أم يؤجل خارج الإصدار الأول؟
15. ما tolerances المقبولة لقياس A/V presentation على speaker/headphones/Bluetooth/HDMI من دون خلطها بدقة زمن المصدر؟
16. هل يبقى audio output stream يعمل بصمت عند mute كي يحتفظ transport بساعة الجهاز، أم ينتقل إلى monotonic clock؟
17. ما سياسة transport عند audio underrun أو route/device change: pause/re-prime أم استمرار مع discontinuity معلنة؟
18. من هو المستخدم المحدد للنسخة المدفوعة الأولى: صانع فيديو قصير Agent-native، أم motion designer، أم محرر فيديو عام؟
19. هل يقتصر Desktop v1 على macOS Apple Silicon وWindows x64، أم يدخل Intel Mac في بوابة الإصدار الأولى؟
20. هل يكون التوزيع الأول مباشرًا من الموقع، أم عبر Microsoft/Mac App Stores، أم مسارين؟
21. هل نموذج العائد الأول Founder Early Access أم اشتراك أم ترخيص دائم مع Agent usage منفصلة؟
22. هل نعتمد Qt Commercial أم مسار LGPL منضبط، وما أثر كل خيار على المتاجر والـSDK؟
23. هل يكون v1 Plugin-ready فقط مع SDK عام مؤجل، أم توجد حاجة تجارية حقيقية لـWasm SDK Preview مبكر؟
24. ما الصيغة النصية النهائية لملفات المشروع التي سيعدّلها Codex مباشرة، وما حدود الملفات الثنائية أو المولدة؟
25. هل يكون commit marker إلزاميًا لكل تعديل Agent متعدد الملفات، أم نوفر أيضًا ingestion محافظًا لحفظ محرر نصي عادي؟
26. ما سياسة التعارض إذا غيّر المستخدم الخاصية نفسها من الـInspector أثناء إعداد الأيجنت لتعديل مبني على Revision أقدم؟
27. هل يدخل multi-selection وmixed values في v1 أم يؤجل بعد تثبيت single-selection Inspector؟
28. ما قياسات الشريط الأيسر والـInspector والـTimeline التي تنجح على أصغر نافذة Desktop مستهدفة؟
29. هل تكون واجهة المنتج عربية/RTL أولًا، أم ثنائية الاتجاه من أول Gate مع تثبيت اتجاه الإحداثيات وأسماء الخصائص؟
30. هل تحفظ Stable IDs داخل ملفات الكيانات نفسها أم في metadata sidecars ملازمة لها؟
31. هل يكون Explicit Agent ChangeSet إلزاميًا في v1 أم يبقى direct-save reconciliation مسار توافق مدعومًا بالكامل؟
32. هل نعتمد `CompositionSpace` من أعلى اليسار وبوحدة pixel واحدة، وما دلالة anchor/pivot الافتراضية لكل Layer؟
33. ما budgets القصوى من file commit إلى active revision لتعديل property وasset وshader وC++ extension؟
34. هل فشل Asset جديدة يرفض ChangeSet كلها أم يسمح بـ`UnresolvedNode` محلي وفق severity policy؟
35. ما مدة الاحتفاظ بـRun Bundles وDiagnostics، وما مستوى redaction الافتراضي قبل مشاركة evidence؟
36. هل تكون `AGENTS.md` هي التعليمات التلقائية الوحيدة، مع `CODEX.md` و`CLOUD.md` كأدلة مرتبطة فقط؟
37. هل يدخل MCP المحلي read/validate/commit/render-probe في v1 أم يبدأ الـCLI وحده مع حجز العقود؟

---

## 19. غربلة فصل الفيديو والصوت واستوديو الصوت

> **حالة هذا القسم: غربلة أولية غير معيارية.** لا يحدد هذا القسم Schema نهائية، ولا ترتيب DSP معتمدًا، ولا مكتبة صوت ملزمة.

### 19.1 الهدف الوظيفي

عند إدراج ملف فيديو يحتوي مسارًا صوتيًا، يجب أن يرى المستخدم افتراضيًا:

```text
V1  ┌──────────────────── Video Clip ────────────────────┐
    └─────────────────────────────────────────────────────┘

A1  ┌──────────────────── Audio Clip ────────────────────┐
    │  waveform L/R حقيقية مرتبطة بزمن المصدر            │
    └─────────────────────────────────────────────────────┘
```

يبدأ العنصران متزامنين ومترابطين للتحريك والقص، لكنهما يبقيان كيانين مستقلين يمكن اختيار أحدهما وتعديله وحذفه وكتمه وإضافة Effects له من دون تحويل الصوت إلى خاصية مخفية داخل Video Layer.

الهدف النهائي هو دعم مستويين من العمل:

1. تحرير سريع داخل Timeline الرئيسية.
2. فتح Audio Clip أو Track أو Bus داخل Audio Studio متخصص للاستعادة والمكساج والتحليل والأتمتة.

### 19.2 التمييز الأساسي: الفصل المنطقي لا الاستخراج الفيزيائي

ملف MP4 أو MOV قد يحتوي video stream وواحدًا أو أكثر من audio streams. الفصل الاحترافي المبدئي هو:

```text
One immutable MediaAsset
├── VideoStream 0
├── AudioStream 1
├── AudioStream 2
└── Metadata/Time bases

Timeline instances
├── VideoClip → MediaAsset.VideoStream 0
└── AudioClip → MediaAsset.AudioStream 1
```

لا يلزم إنشاء WAV منفصل عند كل import كي يصبح الصوت قابلًا للتحرير. يفك Media Engine الـaudio stream مستقلة إلى PCM عند التشغيل أو التحليل، وتبقى أي PCM cache أو proxy ملفات مشتقة قابلة لإعادة البناء.

إنشاء ملف صوت فعلي يبقى عملية صريحة مثل:

- Export Audio.
- Render and Replace.
- Freeze/Commit Effects.
- Generate proxy أو conform cache.

ولا يكون هو السلوك الافتراضي عند إدراج الفيديو.

### 19.3 النموذج المرجح للـTimeline — Linked Component Clips `L + E`

الاتجاه الأقوى حاليًا هو إنشاء Clip لكل media component مع `link_group_id` مشترك:

```text
VideoClip
├── clip_id
├── source_asset_id
├── source_stream_id
├── timeline_range
├── source_range
├── time_mapping
├── video_effect_stack
└── link_group_id

AudioClip
├── clip_id
├── source_asset_id
├── source_stream_id
├── timeline_range
├── source_range
├── time_mapping
├── channel_mapping
├── clip_gain
├── fades
├── audio_effect_stack
└── link_group_id
```

هذا شكل استكشافي، وليس Schema معتمدة.

الخصائص المرغوبة في سلوك الربط:

- الإدراج ينشئ VideoClip وAudioClip في Revision ذرّية واحدة.
- الاختيار العادي يحدد العناصر المرتبطة معًا عندما يكون Linked Selection فعالًا.
- Move وTrim وSplit وRipple وDelete تحافظ على التزامن افتراضيًا.
- Audio properties وAudio Effects لا تنتقل إلى VideoClip.
- Video properties وVideo Effects لا تنتقل إلى AudioClip.
- Modifier مثل Alt/Option يسمح بتحديد مكوّن واحد مؤقتًا.
- Unlink يزيل coupling التحريري من دون تغيير المصدر أو حذف أي Clip.
- Relink يسمح بربط عناصر متوافقة من جديد.
- إذا خرج مكوّنان مرتبطان عن التزامن، يظهر signed sync offset بالتicks والعينات/الفريمات.
- أمر Synchronize يعيد أحد المكوّنين إلى العلاقة المرجعية، مع تحديد أيهما anchor.

توثيق Premiere الحالي يصف المبدأ نفسه: video/audio clips مرتبطة تتصرف كوحدة واحدة، ويمكن Unlink لتحريرها بصورة مستقلة، كما يمكن ربط فيديو واحد بعدة audio clips. هذا دليل منتج للمقارنة، لا مواصفة يُنسخ سلوكها حرفيًا.

### 19.4 أمر الإدراج — شكل بحثي فقط

يفضل أن يكون الإدراج Command دلالية واحدة، مثل:

```text
InsertMedia {
    asset_id,
    target_composition_id,
    at_tick,
    selected_video_stream,
    selected_audio_streams,
    create_linked_components: true,
    audio_channel_policy: "preserve"
}
```

إذا نجح التحقق، تنتج Revision واحدة تحتوي:

- VideoClip على أول Video Track مناسب.
- AudioClip على Audio Track مناسب أسفلها بصريًا.
- LinkGroup يصف علاقة المزامنة.
- Track/channel format متوافقًا مع المصدر أو policy المشروع.

إذا فشل audio decode أو كان الملف بلا صوت، لا ينشئ المحرك AudioClip وهمية. يسجل تشخيصًا واضحًا ويضيف المكونات المتاحة فقط وفق سياسة قبول يجب حسمها لاحقًا.

### 19.5 تعدد المسارات والقنوات

يجب عدم افتراض أن كل فيديو يحتوي Stereo track واحدة. الحالات المطلوبة للغربلة:

- Mono.
- Stereo interleaved.
- Dual mono.
- 5.1 و7.1.
- Ambisonics/immersive formats لاحقًا.
- أكثر من audio stream، مثل لغات مختلفة أو production mix.
- مسارات تعليق أو timecode/reference audio.

السياسات المرشحة عند الإدراج:

| السياسة | السلوك | الحالة |
|---|---|---|
| Preserve | AudioClip متعددة القنوات مع channel map محفوظة | `L + E` كافتراضي |
| Split Channels | إنشاء clips/tracks منفصلة لكل قناة مختارة | `C` كخيار مستخدم |
| Mix Down | تحويل إلى mono/stereo عند الإدراج | `X` كافتراضي، `C` كعملية صريحة |
| Select Streams | اختيار stream أو streams محددة من الملف | `L + E` |

دليل Fairlight يوضح أهمية اختيار clip format وتوزيع القنوات، بما في ذلك وضع قنوات منفصلة على tracks متعددة أو الاحتفاظ بها داخل multichannel adaptive clip. هذا يؤكد ضرورة وجود Channel Mapping صريحة بدل flatten مبكر.

### 19.6 الـWaveform الحقيقية — `L + E`

الـWaveform ليست صورة زخرفية ولا بيانات مستخرجة من حجم packets المضغوطة. يجب أن تُشتق من PCM decoded samples لكل قناة.

المسار المرجح:

```text
Compressed Audio Stream
          │
          ▼
Decode to canonical PCM analysis format
          │
          ▼
Per-channel sample analysis
          │
          ▼
Multi-resolution waveform pyramid
          │
          ▼
Timeline draws only visible tiles/level
```

كل waveform point مرشح لأن يخزن:

- Minimum sample.
- Maximum sample.
- RMS أو energy summary.
- Sample count.
- اختيارياً flags للـclipping أو invalid samples.

يُحفظ cache بمفتاح مشتق من:

```text
asset_content_hash
+ stream_id
+ decoder/version policy
+ channel_mapping
+ waveform_format_version
```

المتطلبات:

- Pyramid متعددة الدقة كي تعمل من عرض المشروع كله إلى مستوى العينات.
- بيانات مستقلة لكل قناة، لا waveform مدمجة فقط.
- Tiled/chunked generation كي تظهر النتائج تدريجيًا بعد الإدراج.
- Cancellation وpriority للمنطقة المرئية.
- عدم حظر قبول Revision أثناء التحليل.
- إعادة بناء cache عند تغير الأصل أو decoder policy.
- ربط كل نقطة بـsource sample index/time، لا بـpixel position.
- عند zoom شديد، يمكن عرض عينات PCM أو approximation أدق بدل تكبير صورة.

مشروع BBC `audiowaveform` يبرهن عمليًا نموذج توليد waveform data بعدد input samples لكل output point، ودعم split channels، وإعادة استخدام ملف البيانات بدل فك الصوت لكل رسم. لا يعني ذلك اعتماده كمكتبة؛ هو مرجع خوارزمي وتجريبي داخل الغربلة.

### 19.7 Source Waveform أم Processed Waveform؟

الترجيح الأولي:

- تعرض Timeline افتراضيًا Source waveform بعد channel mapping الأساسي.
- يظهر clip gain وfades وautomation كـoverlays فوقها.
- لا يعاد بناء waveform المصدر عند تغيير EQ أو compressor أو denoise.
- يمكن طلب Processed waveform preview كcache مشتقة منفصلة عندما يحتاج المستخدم ذلك.
- Render and Replace ينشئ Asset مشتقة جديدة وwaveform جديدة، مع بقاء الأصل وتاريخ العملية معروفين.

سبب هذا الترجيح هو أن إعادة تحليل waveform بعد كل تعديل parameter ستجعل UI بطيئة وغير مستقرة، وستخلط بيانات المصدر بحالة الـeffects المتغيرة.

### 19.8 مستويات Audio Studio

عبارة «فتح التراك في استوديو الصوت» تحتاج التمييز بين ثلاثة سياقات:

#### Clip Studio

لتحرير AudioClip المحددة:

- Source in/out وslip.
- Channel mapping.
- Clip gain وfades.
- Denoise/declick/dehum/declip.
- EQ وdynamics.
- Time stretch وpitch.
- Spectral view/editing لاحقًا.
- Clip automation.

#### Track/Mixer Studio

لمعالجة كل clips المارة عبر track:

- Track inserts.
- Pan وvolume.
- Sends.
- Solo/mute/record arm عند دعم التسجيل.
- Metering.
- Track automation.

#### Bus/Master Studio

- Submix buses.
- Master effects.
- Loudness metering.
- True peak.
- Delivery targets.
- Output channel layout.

فتح Audio Studio لا ينشئ نسخة مشروع أخرى. هو projection متخصصة للـAccepted Project Revision نفسها، مثل Timeline وInspector وCanvas.

```text
UI Audio Studio Command ───┐
                          ├──> ReFusion Command Engine
Agent Audio Command ───────┘
                                   │
                                   ▼
                         Accepted Project Revision
                                   │
                     ┌─────────────┴─────────────┐
                     ▼                           ▼
               Audio Studio                Main Timeline
```

### 19.9 Audio Processing Graph — الاتجاه المرجح `L + E`

أفضل اتجاه حاليًا هو ReFusion Audio Graph مملوكة للمحرك، وليست سلسلة Effects مكتوبة في UI ولا state مخفية داخل مكتبة طرف ثالث.

شكل استكشافي:

```text
Audio Source Stream
        │
        ▼
Decode / PCM Cache
        │
        ▼
Channel Mapping
        │
        ▼
Clip Processing Graph
Gain / Fades / Restoration / EQ / Dynamics
        │
        ▼
Time/Pitch Processing
        │
        ▼
Track Processing Graph
        │
        ├──> Sends ──> Buses
        ▼
Track Mix
        │
        ▼
Master Bus
Loudness / True Peak / Delivery
        │
        ├──> Realtime audio device
        └──> Offline export/mux
```

ترتيب clip gain وrestoration وtime stretch والـeffects في هذا الرسم غير معتمد ويحتاج غربلة منفصلة؛ فالترتيب يغير النتيجة والصوت والـlatency.

متطلبات graph الأولية:

- Planar `f32` processing كحد أدنى، مع دراسة `f64` لبعض المعالجة/offline paths.
- Sample-accurate time وautomation.
- Explicit channel layouts.
- Real-time safe processing بلا allocations أو locks غير محدودة في audio callback.
- Offline rendering يستخدم العقد الدلالية نفسها.
- Plugin/effect latency reporting وautomatic delay compensation.
- Tail handling للـreverb/delay.
- Bypass لا يكسر التوقيت.
- Deterministic parameter snapshots مرتبطة بالـRevision.
- Separate control thread وaudio render thread.
- Diagnostics عند overload/dropout أو unsupported layout.

معيار VST3 يضع automation points داخل processing blocks لتمكين sample-accurate reconstruction؛ وهو دليل مهم على أن Audio Graph لا يمكن أن تعتمد على تحديثات UI منخفضة التردد فقط.

### 19.10 Effects كـCapabilities

كل Audio Effect يجب أن يطبق النموذج نفسه المستخدم في ReFusion:

```text
AudioEffectCapability
├── stable_id
├── version
├── supported_channel_layouts
├── sample_rate_contract
├── parameters/schema
├── automation support
├── reported latency
├── tail duration
├── realtime/offline support
├── platform support
├── processor implementation
├── inspector/editor schema
└── diagnostics
```

وبذلك يستطيع الأيجنت تنفيذ أمر مثل:

```text
AddAudioEffect {
    target: AudioClip("audio-01"),
    effect_id: "refusion.audio.denoise.spectral.v1",
    parameters: {
        reduction_db: 9.0,
        preserve_transients: true
    }
}
```

ولا يستطيع اختراع effect أو parameter غير مسجلة.

### 19.11 غربلة محرك الصوت والمكتبات

#### FFmpeg/libavfilter كمحرك Audio Studio كامل — `X` للدور الأساسي، `C` كمزوّد محدود

نقاط القوة:

- filter graph عام للصوت والفيديو.
- عدد كبير من audio filters، مثل compressor وdeclick وdeclip وresample وغيرها.
- مفيد في decode، conversion، offline operations وبعض implementations المغلفة.

سبب الاستبعاد المبدئي من دور «الحقيقة الأساسية لاستوديو الصوت»:

- يجب أن تكون خصائص ReFusion والـautomation والـlatency والـInspector والـRevision هي العقد العليا، لا syntax أو state خاصة بـFFmpeg filter graph.
- لا نريد تسرب أسماء وأنواع وتوقيت FFmpeg إلى Project Document.
- احتياجات realtime editing والـplugin hosting وsample-accurate automation وUI lifecycle تحتاج عقدًا يملكها ReFusion.

الترجيح: استخدام FFmpeg خلف Media/Effect adapters عند فائدته، لا جعل libavfilter هو Project Audio Engine.

#### JUCE كمحرك كامل — `C + E`

نقاط القوة:

- AudioProcessorGraph واضح للعقد والتوصيلات.
- دعم واسع للأجهزة والمنصات وصيغ plugins.
- مناسب لبناء/استضافة VST3 وAU/AUv3 وLV2 حسب المنصة.

المخاطر:

- نموذج ترخيص حالي dual licensed تحت JUCE licence وAGPLv3، ويحتاج قرارًا تجاريًا/قانونيًا.
- C++ framework كبير قد يتداخل مع قرار Rust core وQt UI.
- اعتماد graph الداخلية مباشرة قد يجعل استبدالها صعبًا.

الترجيح المؤقت: لا اعتماد كامل الآن. يمكن دراسته كـAudio I/O وplugin-host adapter خلف ReFusion Audio API إذا أثبتت التجربة والترخيص ملاءمتهما.

#### GStreamer pipeline كمحرك أساسي — `X` مبدئيًا

يمتلك pipeline مبنية من source/filter/sink elements، لكنه يكرر جزءًا كبيرًا من Media Engine ويفرض lifecycle/state graph خاصة به. لا يظهر حاليًا سبب قوي لإضافة Media framework ثانية إلى جانب FFmpeg/native adapters ومحرك ReFusion.

#### Custom ReFusion Audio Graph + adapters — `L + E`

الاتجاه المرجح:

- Project/Command schema مملوكة لـReFusion.
- DSP node ABI مملوكة لـReFusion.
- built-in effects منتقاة أو مخصصة خلف capabilities.
- FFmpeg للdecode/resample/export وبعض المعالجات.
- Native audio device adapters للمنصات.
- JUCE أو host adapters اختيارية لاحقًا للـplugins، من دون جعلها حقيقة المشروع.

الخطر الأساسي هو كلفة بناء audio graph احترافية، ولذلك يجب تحديد نطاق رأسي صغير وإثبات realtime/offline parity قبل التوسع.

### 19.12 إزالة الضوضاء و«رفع الدقة»

يجب عدم وضع كل عمليات التحسين تحت زر غامض واحد. الغربلة المبدئية تفصلها إلى:

| Capability | الاستخدام | المرشحون/الطريقة | الحالة |
|---|---|---|---|
| Spectral Denoise | ضوضاء ثابتة وعامة مع noise profile | STFT + learned/profile noise estimate | `C + E` |
| Speech Denoise | كلام مع ضوضاء محيطية | RNNoise، DeepFilterNet، مرشحون آخرون | `C + E` |
| Dehum | 50/60Hz والتوافقيات | adaptive notch filters | `C + E` |
| Declick/Decrackle | نقرات وتشققات | transient detection/interpolation | `C + E` |
| Declip | استعادة قمم مقصوصة | waveform reconstruction | `C + E` |
| Dereverb | تقليل صدى الغرفة | DSP/ML | `C + E` |
| Enhance Speech | وضوح الكلام والتوازن | chain أو ML model | `C + E` |
| Bandwidth Extension | تقدير ترددات مفقودة | ML/offline processing | `C + E` |
| Resampling | تغيير sample rate تقنيًا | high-quality resampler | `C`، وليس استعادة تفاصيل |

ملاحظة مهمة:

> رفع sample rate من 44.1kHz إلى 96kHz لا يعيد معلومات مفقودة ولا «يرفع دقة» التسجيل بذاته. إذا كان المطلوب استعادة وضوح أو ترددات مفقودة، فهذا يسمى restoration أو bandwidth extension ويجب عرضه بصدق مع Diagnostics وحدود واضحة.

RNNoise مرشح خفيف للكلام ومتاح بترخيص BSD-3-Clause، لكنه موجه أساسًا إلى speech enhancement ولا يمثل denoiser عامًا للموسيقى والمؤثرات. DeepFilterNet يقدم full-band speech enhancement ومسار Rust ويدعم معالجة real-time، بترخيص MIT أو Apache-2.0، لكنه يحتاج اختبار جودة وأداء ومنصات ومراجعة أحجام النماذج.

لا يُعتمد أي نموذج AI قبل اختبار:

- artifacts على الكلام والموسيقى.
- latency وlookahead.
- CPU/GPU/NPU usage.
- deterministic offline output.
- model licensing وredistribution.
- on-device operation وسياسة الخصوصية.
- behavior عند لغات وأصوات ولهجات مختلفة.

### 19.13 Loudness وMetering

Audio Studio المهنية تحتاج أكثر من volume slider:

- Sample peak.
- True peak.
- Momentary/short-term/integrated loudness.
- Loudness range.
- Per-channel meters.
- Phase/correlation.
- Clipping diagnostics.

EBU R128 توصي بقياس programme loudness وLoudness Range وMaximum True Peak؛ لكن target التسليم الفعلي يختلف حسب broadcast/streaming/cinema والمنصة. لذلك يجب تسجيل delivery preset كCapability، لا hard-code قيمة واحدة لكل المشاريع.

### 19.14 الخيارات المستبعدة مبدئيًا

#### X-01 — Audio كحقل داخل VideoClip فقط

مستبعد لأنه يمنع الاستقلال الحقيقي للقص والـeffects والـrouting والـautomation، ويجعل Audio Studio واجهة شكلية فوق كيان فيديو.

#### X-02 — إنشاء ملف WAV جديد تلقائيًا لكل فيديو

مستبعد كسلوك افتراضي لأنه:

- يضاعف التخزين.
- يخلق سؤال ownership ومزامنة بين أصلين.
- يبطئ الإدراج.
- لا حاجة له كي يصبح audio stream قابلًا للتحرير.

يبقى WAV/PCM cache مشتقة أو export صريحًا عند الحاجة.

#### X-03 — دمج video/audio في Clip واحدة بصريًا بلا Audio Track حقيقية

مستبعد لأن المستخدم لا يستطيع routing أو mixer أو track effects أو channel layouts بصورة مهنية.

#### X-04 — Waveform PNG ثابتة

مستبعد لأنها لا توفر zoom حقيقيًا، ولا قنوات مستقلة، ولا sample-accurate mapping، وتصبح غير صالحة عند trim/time remap.

#### X-05 — معالجة صوتية destructive كافتراضي

مستبعد لأن التعديل يجب أن يكون قابلًا للتراجع وإعادة الضبط والمقارنة. Render and Replace يبقى أمرًا صريحًا يولد أصلًا مشتقًا.

#### X-06 — استخدام waveform بعد المعالجة كمصدر حقيقة

مستبعد لأن الـwaveform cache مشتقة وليست audio source ولا Project truth.

### 19.15 مصفوفة غربلة أولية

الدرجات فرضيات من `1` إلى `5` وليست نتائج قياس.

| النموذج | استقلال التحرير | حفظ التزامن | تعدد القنوات | كلفة التخزين | Audio Studio | الحكم المؤقت |
|---|---:|---:|---:|---:|---:|---|
| Audio داخل VideoClip | 1 | 5 | 2 | 5 | 1 | `X` |
| استخراج WAV تلقائي + أصلان مستقلان | 4 | 2 | 4 | 1 | 4 | `X` كافتراضي |
| MediaAsset واحدة + component clips مرتبطة | 5 | 5 | 5 | 5 | 5 | `L + E` |
| Clip واحدة مرئية مع subcomponents مخفية | 2 | 5 | 3 | 5 | 2 | `X` للـTimeline المهنية |

| Audio engine candidate | Project truth control | Realtime graph | Offline parity | Plugin ecosystem | License simplicity | الحكم |
|---|---:|---:|---:|---:|---:|---|
| FFmpeg/libavfilter كامل | 2 | 3 | 5 | 1 | 2 | `X` كأساس، `C` كمزوّد |
| JUCE كامل | 3 | 5 | 4 | 5 | 2 | `C + E` |
| GStreamer كامل | 2 | 4 | 4 | 3 | 3 | `X` مبدئيًا |
| ReFusion graph + adapters | 5 | 4 مفترضة | 5 مستهدفة | 4 لاحقًا | 4 حسب adapters | `L + E` |

### 19.16 التجارب المطلوبة

#### E-009 — Atomic Linked Insert

استيراد ملفات تحتوي:

- video + mono.
- video + stereo.
- video + 5.1.
- video + multiple audio streams.
- video بلا audio.

ثم إثبات أن Revision واحدة تنشئ المكونات الصحيحة، وأن move/trim/split/unlink/relink/undo/redo لا تكسر المزامنة.

#### E-010 — Real Waveform Pyramid

- بناء waveform تدريجيًا لملف طويل.
- zoom من ساعات إلى sample level.
- split-channel display.
- tiles مرئية فقط.
- إلغاء وإعادة استكمال التحليل.
- تحقق أن peaks لا تخفي clipping بسبب downsampling.

#### E-011 — Audio/Video Sync Under VFR and Seeking

اختبار scrub وseek وplayback وexport مع VFR وB-frames وnon-zero PTS، وقياس الانحراف بعينات الصوت والتicks.

#### E-012 — Realtime/Offline DSP Parity

نفس Audio Graph ونفس automation تنتجان النتيجة نفسها دلاليًا في realtime preview وoffline export مع اختلاف block sizes.

#### E-013 — Latency Compensation

سلسلة Effects ذات latencies مختلفة على tracks وbuses، مع parallel paths، ثم إثبات alignment وtail handling بعد bypass والتغيير.

#### E-014 — Denoise Bake-off

مقارنة spectral denoise وRNNoise وDeepFilterNet ومرشحين آخرين على:

- كلام عربي ولهجات مختلفة.
- موسيقى.
- ضوضاء ثابتة ومتحركة.
- reverb.
- transient sounds.
- desktop/mobile performance.

التقييم يجب أن يجمع قياسات واختبار استماع blind، لا الاعتماد على demo واحدة.

#### E-015 — Multichannel Routing

Preserve وsplit وremap وmixdown وtrack/bus layouts، مع export يتحقق من ترتيب القنوات والـmetadata.

### 19.17 الحكم المؤقت لهذه الجولة

> الاتجاه الأقوى هو MediaAsset واحدة غير قابلة للتعديل تحتوي streams الأصلية، مع VideoClip وAudioClip مستقلتين على tracks منفصلة ومرتبطتين افتراضيًا بواسطة LinkGroup. الـWaveform cache حقيقية متعددة الدقة مشتقة من PCM لكل قناة. Audio Studio هي واجهة أخرى للـProject Revision نفسها، وتستخدم ReFusion Audio Graph غير هدامة، مع FFmpeg وnative/JUCE/plugin adapters كمكونات قابلة للغربلة لا كمصدر حقيقة.

ما يزال هذا الحكم `L + E` ولا يصبح قرارًا حتى نجاح E-009 إلى E-013 على الأقل ومراجعة المستخدم لسلوك الربط والقنوات.

---

## 20. غربلة مسار Native Hardware Video إلى GPU

> **حالة هذا القسم: غربلة Draft 0.3 غير معيارية وغير قابلة للاستخدام كمواصفة تنفيذ.** الأسماء والبنى التالية توضح العقود المطلوبة ولا تعتمد API داخلية نهائية.

### 20.1 إعادة صياغة الشرط بصورة قابلة للإثبات

الهدف المطلوب ليس «ألا يلمس CPU أي بايت من الفيديو»، فهذا غير ممكن عند قراءة الملف وفهم الحاوية وتمرير الـbitstream المضغوط. الهدف المهني القابل للقياس هو:

```text
No software video decode
No decoded-pixel CPU mapping
No CPU YUV→RGB conversion
No CPU readback/upload loop
No UI-owned video frames or frame queues
No silent fallback
```

وبصياغة موجبة:

```text
Compressed bytes on CPU/storage side
        │
        ▼
Native hardware video decoder
        │
        ▼
Native GPU/video surface: NV12/P010/other YUV
        │
        ▼
Live texture/external-memory binding
        │
        ▼
GPU color conversion + effects + compositing
        │
        ▼
Engine-owned native presentation surface
```

التسمية الأدق لهذا العقد هي:

> **Zero CPU Pixel Copy with Hardware-Only Decode**

وليست «zero copy مطلقة». كل Effect أو compositing pass يقرأ texture ويكتب texture أخرى داخل GPU، وقد يحتاج backend إلى GPU-only resolve أو format conversion. هذه عمليات GPU مشروعة؛ المحظور هو إخراج pixels إلى CPU ثم إعادتها.

### 20.2 حدود لا يمكن تحويلها إلى وعود مطلقة

#### لا يمكن ضمان hardware decode لكل ملف على كل جهاز

الدعم يتغير حسب:

- Codec.
- Profile وLevel.
- Bit depth وchroma subsampling.
- Resolution وframe rate وbitrate.
- HDR/alpha/interlace configuration.
- الجهاز والـdriver وإصدار النظام.
- عدد جلسات decoder المتزامنة.
- توفر موارد hardware video engine لحظة التشغيل.

Apple توثق أن `RequireHardwareAcceleratedVideoDecoder` يفشل في إنشاء الجلسة إذا كان format/configuration غير مدعوم أو موارد hardware decoder مشغولة. Android يتيح فحص `isHardwareAccelerated()` وPerformance Points لكن النتائج تعتمد على الجهاز والـvendor image. Windows يفرض capability queries أيضًا.

السياسة الوحيدة المطابقة لشرط «لا software decode» هي:

```text
Hardware decode succeeds → play
Hardware decode unavailable → explicit diagnostic / media offline
Never silently fall back to CPU
```

#### لا يمكن إلغاء كل الـqueues والـpools

Hardware decoders تحتاج داخليًا إلى:

- Reference frames.
- B-frame reordering.
- Decoder output surfaces.
- Backpressure.
- Presentation buffering.
- Swapchain images.

وثائق Microsoft تصف decoder surface arrays وتذكر احتياج decoder والـdeinterlacing والـbuffering، مع أمثلة bounds من 3 إلى 32 surface. Apple تصف `CVPixelBufferPool` وإعادة تدوير IOSurfaces وتحذر من حجب output callback لأنه يولد backpressure.

إذن المحظور ليس وجود queue مطلقًا؛ المحظور هو:

> وجود أي frame queue أو presentation decision أو ownership للـvideo surfaces داخل UI أو Timeline view.

الـqueues الضرورية تكون bounded، داخل Media/Render Engine فقط، وتخضع لPTS والـepoch والـbackpressure.

#### لا يمكن ضمان «zero lag» حرفيًا

حتى مع hardware decode توجد أسباب latency حقيقية:

- Storage/network latency.
- Long-GOP seek الذي يحتاج الرجوع إلى keyframe وفك reference frames.
- Decoder initialization.
- B-frame reorder delay.
- GPU effects وresolution العالية.
- VSync/display scheduling.
- Thermal throttling وresource contention.

العقد المهني الصحيح هو قياس latency ووضع budgets وpercentiles ومنع التوقفات القابلة للتجنب، لا كتابة وعد صفري غير قابل للاختبار.

### 20.3 حد السلطة بين UI والمحرك — `P + E`

الـUI ليست Media Pipeline ولا Playback Controller. الحدود المطلوبة:

```text
UI/QML
├── displays immutable engine snapshots
├── sends semantic commands
├── forwards pointer/keyboard input
└── hosts a native viewport rectangle/window

ReFusion Engine
├── owns project and session state
├── owns transport clock
├── maps ticks to source PTS
├── owns demux/decode scheduling
├── owns decoder surface pools
├── owns GPU resources and synchronization
├── evaluates layers/effects
└── owns presentation timing
```

الـUI لا تقوم بأي من الآتي:

- تحديد الفريم المطلوب ذاتيًا.
- تحويل playhead pixel position إلى مصدر زمن نهائي.
- استدعاء decoder مباشرة.
- الاحتفاظ بـdecoded frame.
- إنشاء video frame cache.
- إدارة frame queue.
- اختيار frame عند presentation deadline.
- تقرير drop/repeat.
- امتلاك swapchain أو GPU device الخاص بالـCanvas.
- تنفيذ YUV conversion أو effects.

### 20.4 Project Commands مقابل Transport Commands

لا يلزم أن ينتج Play/Pause/Seek Project Revision دائمة، لكن يجب ألا تديرها UI محليًا. الاتجاه الاستكشافي:

```text
UI Command ───────┐
Agent Command ────┼──> ReFusion Command Gateway
System Command ───┘              │
                                 ├── Project Command
                                 │       └──> Accepted Project Revision
                                 │
                                 └── Transport Command
                                         └──> Engine Session Epoch/State
```

مثال:

```text
SeekTransport {
    composition_id,
    target_tick,
    request_id
}
```

المحرك وحده يحول `target_tick` إلى source PTS ويقرر seek/decode/present. عند Seek جديدة يزيد `decode_epoch` ويلغي نتائج الطلبات القديمة كي لا يصل فريم متأخر إلى Canvas بعد تغيير playhead.

هذه الفكرة تحتاج قرارًا لاحقًا لأنها توسع الرسم الأصلي الذي كان يمرر كل Command إلى Project Revision.

### 20.5 المسار المشترك المرجح — `L + E`

```text
MediaAsset URI/File Descriptor
        │
        ▼
Native Demux / Compressed Sample Reader
CPU handles metadata + compressed packets only
        │
        ▼
Hardware-Only Decode Session
        │
        ▼
Native YUV Surface Pool
PTS + duration + color metadata + sync primitive
        │
        ▼
NativeVideoSurfaceBridge
No CPU lock / no readback
        │
        ▼
GPU YUV Sampling / YCbCr Conversion
        │
        ▼
Linear RGBA16F Working Texture
        │
        ▼
Skia Layers + GPU Effect Graph + Composite
        │
        ▼
Engine-owned Swapchain/Presentation Surface
```

ملاحظة: التحويل إلى `RGBA16F` ليس مطلوبًا دائمًا مباشرة؛ يمكن لبعض passes قراءة YUV planes أو external texture. لكن أول عملية تحتاج compositing خطي كامل ستنتج working texture على GPU. لا توجد CPU copy في هذا الانتقال.

### 20.6 Apple: macOS وiOS — `L + E`

المسار المرجح:

```text
AVAssetReader / AVSampleBufferGenerator
        │ compressed CMSampleBuffer
        ▼
VTDecompressionSession
RequireHardwareAcceleratedVideoDecoder = true
        │
        ▼
CVPixelBuffer from CVPixelBufferPool
IOSurface-backed + Metal-compatible
        │
        ▼
CVMetalTextureCacheCreateTextureFromImage
        │ live binding per Y/UV plane
        ▼
MTLTexture planes
        │
        ▼
Metal color/effect/composite passes
        │
        ▼
CAMetalLayer drawable
```

شروط المسار:

- تمرير `kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder = true`.
- فشل الجلسة بدل استخدام software decoder.
- طلب PixelBuffers متوافقة مع Metal.
- عدم استدعاء `CVPixelBufferLockBaseAddress` لمسار الفيديو.
- الاحتفاظ بـCVMetalTexture/CVPixelBuffer حتى انتهاء GPU command buffer.
- تحرير surface lease بعد completion signal كي تعود إلى pool.
- قراءة Y وUV planes مباشرة في Metal shader مع metadata اللون الصحيحة.

وثائق Apple تصف `CVMetalTextureCacheCreateTextureFromImage` بأنها تنشئ live binding إلى `MTLTexture` الأساسية، وتطلب الاحتفاظ بالمرجع حتى انتهاء أوامر GPU. هذا هو الأساس الفعلي لمسار zero CPU pixel copy على Apple.

### 20.7 Windows — مساران داخل shortlist

#### W-01: Media Foundation Hardware-Only + DXGI surfaces — `L + E`

المسار:

```text
Media Foundation byte stream/source
        │
        ▼
Source Reader / hardware MFT only
        │
        ▼
DXGI native decode surface
ID3D11Texture2D or negotiated ID3D12Resource path
        │
        ▼
Direct3D GPU processing/compositing
        │
        ▼
DXGI Swapchain
```

على Windows 11 Version 25H2 يمكن استخدام:

- `MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS = TRUE`.
- D3D device manager مناسب.
- `MF_READWRITE_USE_ONLY_HARDWARE_TRANSFORMS = TRUE`.

توثيق Microsoft ينص أن `USE_ONLY_HARDWARE_TRANSFORMS` يفشل في إنشاء transform chain إذا لم يجد Hardware MFT مطابقًا، بدل اختيار software decoder.

نقاط تحتاج إثباتًا:

- هل يخرج decoder المختار `ID3D12Resource` مباشرة في الأجهزة المستهدفة أم `ID3D11Texture2D`؟
- إذا كان D3D11، هل يمكن ربطه بالمركب النهائي بلا GPU copy أو عبر GPU-only shared resource؟
- دعم NV12/P010 shader-resource bindings.
- مزامنة Media Foundation producer مع Direct3D consumer.

#### W-02: Direct D3D12 Video Decode — `C + E`

المسار الأعلى تحكمًا:

```text
Native container/bitstream parser
        │
        ▼
ID3D12VideoDecoder + Decode Command List
        │
        ▼
ID3D12Resource NV12/P010
VIDEO_DECODE_WRITE
        │ resource barrier/synchronization
        ▼
Shader/Video Process Read
        │
        ▼
D3D12 compositor + swapchain
```

نقاط القوة:

- جهاز وموارد وbarriers صريحة داخل Direct3D 12.
- لا حاجة إلى D3D11→D3D12 bridge إذا نجح المسار كاملًا.
- `CheckFeatureSupport` يفحص profile وresolution وformat وframe rate وbitrate.

المخاطر:

- يحتاج codec-specific bitstream parsing وreference management.
- كلفة تطوير واختبار أكبر بكثير.
- لا يعني وجود D3D12 أن كل codec/profile مدعوم.

الحكم المؤقت على Windows:

> نحتاج Prototype مقارنة W-01 وW-02. لا يجوز إعلان D3D12 direct فائزًا قبل قياس التوافق والاستقرار وكلفة parser، ولا يجوز قبول D3D11 bridge إذا أدى إلى readback أو نسخ GPU متكررة غير مبررة.

### 20.8 Android — `L + E`

المسار المرجح:

```text
AMediaExtractor / compressed access units
        │
        ▼
Explicit hardware AMediaCodec
configured with output ANativeWindow Surface
        │
        ▼
AImageReader / platform BufferQueue owned by engine/platform
        │
        ▼
AImage → AHardwareBuffer
        │ external memory, never CPU-locked
        ▼
VkImage + Android external format/YCbCr sampling
        │
        ▼
Vulkan effects/composite
        │
        ▼
ANativeWindow swapchain
```

شروط المسار:

- تعداد codecs واختيار CodecInfo يثبت `isHardwareAccelerated()` ورفض `isSoftwareOnly()`.
- فحص profile/level/size/rate وPerformance Points حين تتوفر.
- استخدام `COLOR_FormatSurface`/Surface mode، لا ByteBuffer output للفريمات.
- عدم استخدام `getOutputImage()` أو mapping إلى CPU في المسار الأساسي.
- عدم استدعاء `AHardwareBuffer_lock` للفريمات.
- استيراد AHardwareBuffer إلى Vulkan external memory مع sync fences.
- BufferQueue Android داخل platform/engine boundary، وليست queue UI أو Timeline.

وثائق Android تنص أن AHardwareBuffer يمكن ربطها بـVulkan external memory وأن تمريرها يمثل shared view بلا نسخ. لكن اختلاف vendor codecs والأجهزة يعني ضرورة device matrix فعلية، وليس اختبار هاتف واحد.

### 20.9 NativeVideoSurfaceLease — شكل بحثي

لا يدخل native handle في Project Document. تمر resource lease مؤقتة داخل المحرك:

```text
NativeVideoSurfaceLease
├── decode_epoch
├── source_asset_id
├── source_pts
├── duration
├── coded/display size
├── pixel format and plane descriptors
├── color metadata
├── native backend
│   ├── Apple: CVPixelBuffer/CVMetalTexture
│   ├── Windows: ID3D11Texture2D or ID3D12Resource
│   └── Android: AHardwareBuffer/VkImage
├── ready synchronization primitive
├── release/completion primitive
└── pool return callback
```

قواعد الملكية المبدئية:

- Media Engine تنشئ أو تستلم الـlease.
- Render Graph تستعيرها حتى GPU completion.
- UI لا تراها ولا تستطيع الاحتفاظ بها.
- Timeline لا تخزن handle أو pointer.
- surface تعود إلى decoder pool بعد آخر fence فقط.
- Device loss يبطل كل leases بواسطة generation/epoch جديد.

### 20.10 الجدولة والـqueues الضرورية

الترجيح هو scheduler واحد داخل المحرك:

```text
Transport Clock / Frame Evaluator
                │
                ▼
         Frame Request Scheduler
                │
       ┌────────┴────────┐
       ▼                 ▼
Compressed Packet   Decoder Surface Pool
Bounded Queue       Bounded Native Pool
       │                 │
       └────────┬────────┘
                ▼
          PTS Reorder Table
                │
                ▼
          Render Deadline
                │
                ▼
             Present
```

السياسات المطلوبة:

- كل queue bounded بحد معروف ومقاس.
- backpressure بدل نمو الذاكرة.
- `decode_epoch` لإسقاط نتائج Seek القديمة.
- PTS هو معيار العرض، لا ترتيب callback.
- عدم حجز decoder surfaces أثناء انتظار UI.
- preview cache GPU محدودة ومشتقة.
- لا polling من UI ولا QTimer لتقرير الفريم.
- no frame-ready signal يحمل pixels إلى UI؛ أقصى ما يصلها diagnostic/statistics snapshot.

### 20.11 Engine-owned presentation — `L + E`

المسار الأنظف لتحقيق عدم سلطة UI هو أن تكون Canvas نافذة/طبقة native يملك renderer swapchain الخاصة بها:

| المنصة | Presentation owner المرشح |
|---|---|
| macOS/iOS | Engine-managed `CAMetalLayer` |
| Windows | Engine-managed HWND child + DXGI swapchain |
| Android | Engine-managed `ANativeWindow`/Surface + Vulkan swapchain |

Qt/QML تستطيع إدارة موضع الـviewport وبقية panels وتمرير input، لكنها لا تستلم الفريم كـ`QImage` أو `QVideoFrame`.

هناك خيار آخر لعرض RGBA texture native داخل Qt Quick باستخدام native `QSGTexture` wrappers، لكن هذا يبقي تكاملًا مع scene graph/render thread ويحتاج دراسة lifetime وcompatibility. لذلك هو `C + E` وليس المسار المرجح الأول للـCanvas الصارمة.

### 20.12 قائمة المسارات المحظورة مبدئيًا

إذا اعتُمد Hardware-Only Strict Mode لاحقًا، تصبح هذه العمليات محظورة في video playback/render path:

#### CPU/software decode

- أي software video decoder fallback.
- FFmpeg software decoding إلى CPU `AVFrame` للفيديو.
- platform software codec عندما لا يوجد hardware codec.
- fallback صامت عند decoder resource exhaustion.

#### CPU pixel movement

- `av_hwframe_transfer_data` أو `hwdownload` للفريم المعروض.
- `sws_scale` لتحويل YUV→RGB في CPU.
- `CVPixelBufferLockBaseAddress` للوصول إلى decoded video pixels.
- `AHardwareBuffer_lock` للفريمات.
- D3D resource Map/ReadFromSubresource لمسار التشغيل.
- `glReadPixels` أو أي texture readback.
- نسخ decoded frame إلى heap buffer ثم إعادة upload.

#### Qt/UI paths

- `QImage` أو `QPixmap` للفيديو.
- `QPainter` لرسم decoded video frame.
- `QQuickImageProvider` للفريمات.
- `QMediaPlayer`/`QVideoSink`/`QVideoFrame` بوصفها pipeline المشروع.
- UI-owned image/video queue أو frame cache.
- Timeline-owned decoder أو playback clock.
- QML animation/timer بوصفه frame scheduler.

#### Render fallbacks

- Skia raster/`SkBitmap` لمسار Video Layer أثناء التشغيل.
- OpenGL/GLES fallback عندما يكون backend المطلوب Metal/D3D/Vulkan.
- wgpu GL backend في Hardware-Only Strict Mode.
- أي intermediate PNG/JPEG/RGBA CPU image.

#### Authority violations

- تغيير playhead محليًا في UI من دون Transport Command مقبولة.
- اختيار frame بناءً على slider progress.
- امتلاك UI للـPTS أو source time mapping.
- استمرار عرض frame من decode epoch قديمة بعد seek.

الاستثناءات يجب أن تكون أدوات تشخيص offline صريحة لا تدخل المسار المنتج، مثل screenshot اختبارية بطلب مخصص، مع إظهار أنها readback وليست playback path.

### 20.13 Hardware-Only Capability Gate — `L + E`

قبل التشغيل يحتاج المحرك إلى نتيجة صريحة:

```text
HardwareDecodeCapability
├── backend and physical adapter
├── codec/profile/level
├── bit depth/chroma
├── resolution/frame rate/bitrate
├── output pixel formats
├── native-surface import support
├── shader sampling support
├── concurrent session limits if known
├── verified_hardware_only
├── verified_zero_cpu_pixel_path
└── status/diagnostic
```

الحالات المرشحة:

```text
ReadyNativeZeroCpuCopy
ReadyNativeWithGpuResolve
UnsupportedCodecProfile
UnsupportedNativeSurfaceInterop
HardwareResourcesBusy
DriverOrDeviceRejected
DeviceLost
```

لا تعرض UI خيارًا لتجاوز النتيجة. هي تعرض diagnostic صادرًا من المحرك فقط.

### 20.14 غربلة البدائل

| الخيار | Hardware-only | Zero CPU pixel copy | تحكم ReFusion | Cross-platform consistency | الحكم |
|---|---:|---:|---:|---:|---|
| Qt Multimedia pipeline | غير مضمون كعقد ReFusion | غير قابل للإثبات مركزيًا | منخفض | متوسط | `X` كأساس |
| FFmpeg software decode | لا | لا | متوسط | عالٍ | `X` في strict mode |
| FFmpeg hwaccel abstractions | ممكن | يحتاج إثبات لكل backend | متوسط/عالٍ خلف adapter | عالٍ | `C + E` |
| Native APIs + native surfaces | نعم عند نجاح capability gate | الأعلى احتمالًا | عالٍ | يحتاج adapters مستقلة | `L + E` |
| Native decode + CPU RGBA upload | نعم للdecode فقط | لا | عالٍ | متوسط | `X` |
| Engine-owned native viewport | غير منطبق | يحافظ على المسار | عالٍ | يحتاج platform hosts | `L + E` |
| Qt adopted native texture | غير منطبق | ممكن | متوسط | يحتاج lifetime/backend audit | `C + E` |

الترجيح:

> Native APIs + native GPU/video surfaces + engine-owned presentation، مع fail-closed عند غياب hardware أو interop. FFmpeg hardware paths تبقى مرشحًا احتياطيًا للغربلة التقنية، لا software fallback ولا مصدر حقيقة.

### 20.15 ما يعنيه هذا لدعم الصيغ

فرض Hardware-Only Strict يضيق support matrix. لا يجوز الادعاء أن ReFusion «يفتح كل فيديو» ثم منع software fallback في الوقت نفسه.

يلزم اختيار إحدى سياستين لاحقًا:

#### Strict Product Policy — `L` بناءً على طلب المستخدم

- يشغل فقط codec/profile/device combinations التي اجتازت capability gate.
- الملفات الأخرى تُدرج Offline أو تُرفض مع تشخيص.
- يمكن توفير Transcode خارجي/مسبق إلى codec مدعوم، لكنه ليس playback fallback مخفيًا.

#### Dual Mode Policy — `X` حاليًا بناءً على الشرط المطلوب

- Native Hardware Mode مفضل.
- Software compatibility fallback للصيغ الأخرى.

هذا الخيار مستبعد حاليًا لأنه يخالف شرط منع software decode، لكنه يبقى مسجلًا لإظهار المقايضة بوضوح.

### 20.16 الأداء والـlatency

بدل وعد `zero lag`، يجب أن تحدد التجارب لاحقًا حدودًا رقمية لكل جهاز وفئة ملف:

- Decoder startup P50/P95/P99.
- First-frame latency.
- Random seek latency حسب GOP length.
- Sustained 1080p/4K/8K playback.
- Dropped/repeated frames.
- GPU frame time.
- Decoder surface occupancy.
- Memory budget.
- UI responsiveness أثناء seek، من دون أن تدير UI عملية seek.
- A/V sync drift.

تقنيات مرشحة لتقليل التأخير من دون كسر العقد:

- Keyframe/PTS index مشتقة.
- Hardware-decodable edit-friendly proxies.
- Bounded GPU frame cache.
- Asynchronous decode بعيدًا عن callback الحساس.
- cancellation بالـepoch.
- deadline-aware scheduling.
- prewarming decoder sessions عند توفر الموارد.

### 20.17 التجارب المطلوبة

#### E-016 — Apple Zero CPU Pixel Path

على macOS وiOS:

- H.264 وHEVC، 8/10-bit، SDR/HDR حسب الجهاز.
- إثبات hardware-required session.
- CVPixelBuffer/IOSurface إلى Metal planes.
- عدم LockBaseAddress أو CPU upload.
- Metal YUV→linear composite→CAMetalLayer.
- Instrument lifetime وsurface pool pressure.

#### E-017 — Windows Native Decode Bake-off

مقارنة W-01 وW-02:

- Media Foundation hardware-only DXGI path.
- Direct D3D12 Video Decode path.
- NV12/P010 resource sharing والسynchronization.
- D3D11/D3D12 interop cost إن وجد.
- feature/profile/device matrix.
- إثبات failure بدل software fallback.

#### E-018 — Android Hardware Surface Matrix

على عدة vendors وAPI levels وفئات أداء:

- اختيار codec مثبت hardware.
- Surface mode.
- AHardwareBuffer/Vulkan external memory.
- no ByteBuffer output وno CPU lock.
- 4K/10-bit/HDR حيث تدعم capability.
- seek، format changes، decoder reset وthermal stress.

#### E-019 — UI Authority Audit

اختبار معماري/ساكن يثبت:

- UI target لا يرتبط بمكتبات decode.
- لا أنواع native frame في UI module.
- لا QImage/QVideoFrame paths للفيديو.
- لا playback clock أو frame queue في Timeline.
- كل seek/play/pause يمر عبر Command Gateway.

#### E-020 — Zero-Copy Instrumentation

إضافة counters وGPU capture/tests تكشف:

- CPU pixel mappings.
- readbacks/uploads.
- GPU copy/resolve passes.
- native surface import failures.
- unexpected software decoder activation.
- frame lease lifetime وpool starvation.

تنجح التجربة فقط إذا كانت العدادات صفرًا للـCPU pixel transfers في steady-state playback.

#### E-021 — Latency and Stress Gates

- rapid scrubbing.
- long-GOP random seeking.
- multiple simultaneous clips.
- device busy/resource exhaustion.
- window resize/fullscreen.
- device loss/background/foreground على mobile.
- playback مع Effects وMotion Blur.

#### E-022 — Fail-Closed Behavior

ملفات وأجهزة غير مدعومة يجب أن تنتج diagnostics صحيحة بلا crash وبلا software fallback وبلا frame أسود يوحي بالنجاح.

### 20.18 الحكم المؤقت لهذه الجولة

> المسار المرجح هو Hardware-Only Native Decode لكل منصة، مع decoded surfaces تبقى في native GPU/video memory، وتدخل Render Graph بواسطة live texture أو external-memory binding، ثم تعرض عبر swapchain يملكها المحرك. UI لا تمتلك frame أو queue أو clock أو decoder أو قرار presentation. إذا لم تتوفر hardware capability أو native-surface interop، يفشل المسار صراحة ولا يعود إلى CPU.

لكن يجب تسجيل حقيقتين قبل الـMaster Plan:

1. «Zero copy» هنا تعني **صفر نسخ للـpixels بين GPU وCPU**، لا غياب كل GPU render/resolve operations.
2. «Zero lag» ليست ضمانًا ممكنًا؛ البديل المهني هو latency budgets قابلة للقياس وfail-closed capability contract.

لا يصبح هذا الحكم قرارًا حتى نجاح E-016 إلى E-020 على الأقل على المنصات الأربع.

---

## 21. غربلة عقد زمن المصدر للفيديو وصوته المضمّن

> **حالة هذا القسم: غربلة Draft 0.4 غير معيارية وغير قابلة للاستخدام كمواصفة تنفيذ.** الأسماء والبنى والمعادلات التالية تصف ما يجب اختباره قبل اختيار الـAPI النهائية.

### 21.1 النطاق الدقيق

هذا القسم يخص **MediaAsset واحدة عند إدراجها** ومسار الفيديو ومسار الصوت المضمّن المختارين منها. لا يحاول تعريف زمن المشروع كله، ولا مزامنة عدة clips، ولا transitions، ولا time remapping على مستوى composition.

المطلوب هو أن يبقى معنى زمن الملف الأصلي صحيحًا من لحظة الفحص حتى preview وexport:

- تظهر كل صورة في وقت عرضها الحقيقي الموجود في المصدر.
- يبقى الفيديو ذو Variable Frame Rate متغيرًا ولا يحول ضمنيًا إلى Average FPS.
- يعاد ترتيب B-frames بواسطة PTS لا بحسب ترتيب وصول decoder output.
- يبدأ الصوت في إزاحته الحقيقية بالنسبة إلى الفيديو.
- يستهلك الصوت عدد العينات الصحيح مع احترام encoder priming وpadding وskip metadata.
- لا تخترع UI زمنًا ولا frame number ولا sync offset.

### 21.2 ماذا تعني «100%» هنا؟

يجب فصل ضمانين مختلفين:

| المستوى | ما يمكن ضمانه | الحد الواقعي |
|---|---|---|
| دلالة المصدر | اختيار video sample وaudio sample الصحيحين لكل media time باستعمال حساب صحيح/نسبي حتمي | يمكن جعله exact عندما تكون طوابع المصدر وmetadata صالحة |
| العرض الفيزيائي | وصول الصورة إلى الشاشة والصوت إلى السماعة في اللحظة نفسها | لا يوجد ضمان مطلق بسبب vsync وbuffering وdriver وdisplay/audio-device latency؛ يوضع له tolerance مقاس |

إذًا لا تُكتب عبارة `100% realtime` كادعاء عام. العبارة القابلة للاختبار هي:

> **Exact Source-Time Semantics + Measured A/V Presentation Tolerance**

إذا كان الملف ناقص PTS أو متناقضًا، لا يمكن استعادة حقيقة غير موجودة. في الوضع الصارم يجب إصدار Diagnostic أو جعل الأصل timing-offline بدل التخمين الصامت.

### 21.3 مصدر الحقيقة الزمني

مصدر الحقيقة ليس FPS المكتوب في Inspector ولا مدة محسوبة من عدد frames. المصدر هو:

1. دلالة الحاوية، بما فيها edit lists وstream start offsets إن وجدت.
2. `time_base` المستقلة لكل stream.
3. PTS وDTS وduration لكل video sample/packet وفق backend.
4. PTS وsample rate وsample count لكل audio block.
5. priming/padding/skip samples والـdiscontinuities المعلنة.

FFmpeg يعرّف `time_base` كوحدة الزمن الأساسية لطوابع stream، ويعرّف `AVFrame.pts` كوقت العرض و`duration` بالوحدة نفسها، بينما يحمل frame الصوتي `nb_samples`. وعلى Apple توجد presentation/decode timestamps وsample timing، وعلى Android يعيد `MediaCodec.BufferInfo.presentationTimeUs` طابع العرض. هذه أدلة على أن نموذج المصدر يجب أن يكون timestamp-driven لا FPS-driven.

### 21.4 التمثيل العددي المرجح — `L + E`

يجب استخدام زمن نسبي exact مثل:

```text
RationalTime {
    signed_ticks: int64-or-wider
    time_base_num: integer
    time_base_den: nonzero_integer
}
```

مع normalization وفحص overflow وسياسة rounding معلنة عند عبور time bases. يمنع استخدام `float` أو `double` أو milliseconds العشرية كمصدر حقيقة.

لكل sample:

```text
stream_time = timestamp × time_base_num / time_base_den
media_local_time = stream_time - media_presentation_origin
```

تبقى المقارنة والطرح والضرب ككسور صحيحة، ولا تتحول إلى nanoseconds أو microseconds ثابتة إلا عند boundary منصة وبـrounding موثق. إذا احتاجت العمليات مدى أكبر، يكون المرشح الداخلي integer أوسع من 64-bit أو rational checked arithmetic.

### 21.5 MediaTimingManifest مشتقة وليست حقيقة مشروع

عند الإدراج، ينفذ Media Core فحصًا زمنيًا ويبني index قابلة لإعادة البناء، مرتبطة بـasset content hash وstream IDs:

```text
MediaTimingManifest (derived cache)
├── container presentation origin + edit mapping
├── VideoPresentationIndex
│   └── PTS, DTS, duration, keyframe, dependency/discontinuity flags
└── AudioSampleIndex
    └── PTS, sample rate, first-sample index, sample count,
        priming, padding, skip-start, skip-end, discontinuity flags
```

خصائصها المطلوبة:

- versioned وchecksummed.
- لا تعدل المصدر.
- ليست جزءًا من سلطة UI ولا Project truth.
- يمكن حذفها وإعادة بنائها لتعطي النتيجة الدلالية نفسها.
- لا تعتمد `best effort timestamp` أو duration estimated من bitrate من دون وسم صريح بأنها تقديرية.

### 21.6 اختيار frame الفيديو الصحيح

ترتب الصورة للعرض بحسب PTS، لا بحسب DTS ولا ترتيب callback. الصورة `i` صالحة في المجال نصف المفتوح:

```text
[PTS(i), PTS(i) + duration(i))
```

وعند طلب زمن `t` يختار المحرك الصورة التي يحتوي مجالها `t`. هذا يحافظ على VFR؛ فلا توجد معادلة من نوع:

```text
frame = round(seconds × average_fps)   // محظورة
```

إذا غابت duration:

- يجوز اشتقاقها من فرق PTS التالي فقط ضمن سياسة container/codec موثقة ومثبتة بالتجربة.
- لا يجوز استعمال `1 / average_fps` كبديل صامت.
- إذا بقيت ملتبسة في Strict Timing Mode تصبح `TimingAmbiguous`.

عند seek إلى زمن وسط GOP، يجوز للdecoder البدء من keyframe أسبق كي يفك dependencies، لكن لا تعرض تلك frames على أنها زمن الطلب. decode lead-in ليس presentation.

### 21.7 اختيار عينات الصوت الصحيح

الصوت لا يقاس بعدد video frames. لكل audio block يبدأ في `audio_pts` ويحمل `N` عينة عند sample rate `R`:

```text
sample_time(n) = audio_pts + n / R
audio_block_interval = [audio_pts, audio_pts + N / R)
```

يجب أن يحافظ المسار على:

- `nb_samples` لكل قناة.
- sample rate وchannel layout.
- encoder delay/priming.
- end padding وskip samples.
- حالة resampler وتأخيره عبر blocks؛ لا يعاد تشغيله عند كل block.

أي تحويل sample rate أو time-stretch هو Transform صريح لاحق، وليس تصحيحًا خفيًا عند الإدراج.

### 21.8 إزاحة بداية الصوت والفيديو

لا يفترض أن أول PTS للفيديو يساوي أول PTS للصوت. بعد تطبيق دلالة container/edit list، يحسب المحرك:

```text
video_offset = first_presented_video_time - media_presentation_origin
audio_offset = first_audible_audio_time - media_presentation_origin
```

إذا بدأ الصوت بعد الفيديو بـ21 ms مثلًا، تحفظ فجوة 21 ms. وإذا بدأ قبله، تحفظ pre-roll/negative offset وفق دلالة الحاوية. لا تُسحب clip الصوت إلى صفر كي يبدو الشكل متساويًا.

عند الإدراج ينتج Command واحد Revision ذرية فيها VideoClip وAudioClip، وكلاهما يشيران إلى `SourceSyncGroup` واحد وإلى offsets المصدر الحقيقية. `LinkGroup` يحدد سلوك التحرير والتحريك فقط؛ لا يعد مصدرًا للتزامن.

### 21.9 ساعة التشغيل داخل جلسة هذا الأصل — `L + E`

أثناء تشغيل أصل له صوت، المرشح المهني هو:

```text
Audio device/sample clock (master)
              │
              ▼
 measured media presentation time
              │
              ▼
 video PTS selection + vsync scheduling
```

السبب: جهاز الصوت يستهلك سلسلة عينات مستمرة وله clock فعلي. توثق Microsoft أيضًا أن استخدام audio renderer كـtime source مفيد كي يتبع الفيديو معدل تشغيل الصوت.

القواعد المبدئية:

- تقاس موضعية الصوت من hardware/device timestamp أو clock correlation، لا من عدد callbacks المرسلة فقط.
- يجدول الفيديو مقابل زمن الصوت المتوقع عند presentation، مع تعويض latency المعلومة.
- إذا فات frame موعد العرض بسبب ضغط الجهاز، يجوز drop/hold وفق سياسة معلنة؛ لا يُمدد الصوت ولا يُزاح سرًا.
- إسقاط frame عرض لا يغير PTS المصدر ولا نتائج export.
- عند عدم وجود صوت تستخدم جلسة الأصل monotonic media clock مرتبطة بـvsync.
- pause/seek/rate change تنشئ clock epoch جديدة وتلغي نتائج decode القديمة.

هذه الساعة تخص Playback Session لهذا الأصل في هذه الغربلة؛ ليست تعريفًا لساعة المشروع العامة.

بعد إدراج الأصل في مشروع، لا يملك الأصل ولا AudioClip زمن المشروع. ساعة جهاز الصوت تصبح ClockSource يقرأها Project Transport Authority كما هو مغربل في القسم التالي.

### 21.10 Preview مقابل Export

يجب أن يستعمل المساران evaluator الزمني نفسه:

```text
Requested media-local rational time
                 │
                 ▼
       Source Timing Evaluator
          │              │
          ▼              ▼
      Video PTS     Audio sample range
          └──────┬───────┘
                 │
          ┌──────┴──────┐
          ▼             ▼
 Realtime preview   Offline export
  (audio master)      (rateless)
```

Preview مرتبط بساعة فعلية وقد يضطر إلى drop presentation. Export rateless/offline ولا يعتمد wall clock؛ يكتب timestamps من التقييم النسبي exact ويحافظ على offsets وpriming/padding. التطابق المطلوب هو دلالة الزمن والمحتوى المختار، لا أن Preview وExport يعملان بالسرعة نفسها.

### 21.11 UI بلا أي سلطة زمنية

Timeline وInspector وCanvas:

- تعرض `MediaTimingSnapshot` immutable فقط.
- ترسل seek/trim/split commands بوحدة زمن المحرك.
- لا تحسب frame index من FPS.
- لا تختار PTS ولا تقرأ decoder callback.
- لا تقطع audio samples ولا تعالج priming.
- لا تملك playback clock أو sync offset أو frame/audio queue.

محرك الأوامر يتحقق من الزمن، وMedia Timing Evaluator يحوله إلى PTS/sample ranges، وTransport/Render/Audio engines تنفذ. waveform تمثيل مشتق للعرض ولا يصبح مصدر توقيت.

### 21.12 حالات الصحة وFail-Closed

يقترح أن ينتج الفحص واحدة أو أكثر من الحالات التالية:

| الحالة | المعنى المبدئي |
|---|---|
| `TimingValidExact` | PTS/durations/sample counts والـoffsets كافية لاختيار حتمي |
| `TimingValidWithDeclaredEdits` | صالح بعد تطبيق edit-list/skip metadata معلنة |
| `TimingDiscontinuous` | توجد discontinuities معلنة ويجب تمثيل gaps/epochs |
| `TimingAmbiguous` | توجد أكثر من قراءة ممكنة ولا دليل كافٍ للحسم |
| `MissingPTS` | طوابع عرض لازمة غير موجودة |
| `AudioPrimingUnknown` | لا يمكن تحديد البداية المسموعة sample-exact |
| `TimingCorrupt` | طوابع أو مدد متناقضة/خارج المجال |

في Strict Timing Mode:

```text
Valid timing     → import/play with exact contract
Ambiguous timing → explicit diagnostic / timing-offline
Never silently invent FPS, PTS, duration, or A/V offset
```

يمكن لاحقًا غربلة وضع Compatibility منفصل يقبل repair heuristics، لكن إن وجد يجب أن يكون opt-in، موسومًا بوضوح، ولا يدعي exact source timing.

### 21.13 المسارات المستبعدة مبدئيًا لهذا العقد

- `frame_number / average_fps` كمصدر وقت.
- تحويل كل timestamps إلى `double seconds` وحفظها بهذه الصورة.
- تصفير audio وvideo كل على حدة عند الإدراج.
- ترتيب B-frames بحسب decode/callback order.
- استعمال system wall clock وحده مع تجاهل audio device clock.
- جعل waveform أو playhead pixel position مصدرًا للزمن.
- تصحيح drift بإسقاط/إضافة audio samples سرًا.
- إعادة توليد timestamps المفقودة بلا Diagnostic.
- جعل UI تختار frame أو تحسب sync.

### 21.14 التجارب المطلوبة

#### E-023 — Timestamp Conformance Corpus

Corpus موثق يشمل:

- CFR وVFR.
- B-frames وlong GOP.
- non-zero وnegative start timestamps.
- audio يبدأ قبل الفيديو وبعده.
- edit lists وgaps وdiscontinuities.
- 44.1/48/96 kHz وmultichannel.
- priming/padding/skip samples.
- ملفات مفقودة أو متناقضة التوقيت.

تُقارن نتائج inspector الداخلي بالطوابع الفعلية في الحاوية، لا بما تعرضه واجهة مشغل خارجي فقط.

#### E-024 — Deterministic Video Selection

لكل rational time في corpus:

- نفس PTS المختارة في 1000 إعادة تشغيل.
- النتيجة نفسها بعد cold/warm cache.
- النتيجة نفسها في sequential playback وrandom seek.
- اختبار VFR وB-frame ordering على المنصات الأربع.

#### E-025 — Sample-Exact Audio Alignment

ملفات ذات clap/impulse وburn-in timecode، ثم التحقق من:

- أول وآخر audible sample.
- offsets بين الصوت والفيديو.
- priming/padding.
- عدم فقد/تكرار samples عبر seek وblock-size changes.
- parity بين realtime capture وoffline export ضمن ما تسمح به device capture latency.

#### E-026 — Cross-Platform Timing Parity

Apple/Windows/Android backends يجب أن تنتج `MediaTimingManifest` دلالية متكافئة لنفس الملف: نفس presentation intervals ونفس audio sample ranges، حتى لو اختلفت وحدات APIs الأصلية.

#### E-027 — Physical Presentation Measurement

باستخدام high-speed camera أو loopback/photodiode rig عند الحاجة:

- قياس A/V offset وjitter P50/P95/P99.
- قياس أثر vsync وaudio buffer وBluetooth/HDMI route changes.
- وضع tolerance منفصل لكل فئة جهاز ومسار إخراج.
- إثبات أن clock correction لا يغير timestamps المصدر.

### 21.15 الحكم المؤقت لهذه الجولة

> المسار المرجح هو `Source Media Time Contract` قائم على PTS/DTS/duration وaudio sample counts بتمثيل rational/integer exact. عند الإدراج يبني Media Core فهرس توقيت مشتقًا ويحفظ إزاحة كل stream الحقيقية تحت `SourceSyncGroup`. أثناء playback لهذا الأصل يكون audio device/sample clock هو master عند وجود الصوت، ويُجدول الفيديو بحسب PTS؛ أما export فيقيّم الزمن offline بالعقد نفسه. UI تعرض النتيجة ولا تملك الزمن.

بهذا يمكن الادعاء بدقة **دلالة زمن المصدر** للملف السليم. أما التطابق الفيزيائي بين الشاشة والسماعة فيوصف بميزانية tolerance مقاسة، لا بنسبة 100% غير قابلة للإثبات. والملف ذو الزمن الغامض يفشل صراحة في الوضع الصارم ولا يُصلح بالتخمين الصامت.

لا يصبح هذا الحكم قرارًا حتى نجاح E-023 إلى E-027 ومراجعة سياسات edit lists وmissing timestamps وdevice-latency على المنصات الأربع.

---

## 22. غربلة سلطة الزمن الحقيقي للمشروع

> **حالة هذا القسم: غربلة Draft 0.5 غير معيارية وغير قابلة للاستخدام كمواصفة تنفيذ.** الغرض هو فصل إحداثيات الزمن، والجهة المالكة للـtransport، ومصدر النبض الفيزيائي قبل اختيار API نهائية.

### 22.1 الحكم المختصر

بعد إدراج الفيديو، المسؤول عن الزمن الحقيقي للمشروع كله ليس AudioClip ولا Audio Engine ولا Timeline UI. المسؤول المرجح هو:

> **ReFusion Project Transport Authority** داخل Engine Session.

أما Audio Output Device Clock فهو **مصدر الساعة الفيزيائي المفضل** أثناء التشغيل الأمامي مع صوت فعّال. الفرق هو:

```text
ProjectTime             = إحداثيات المشروع ومعناه الزمني
Transport Authority     = المالك الوحيد لحالة التشغيل وتحويل clock إلى ProjectTime
Audio Device Clock      = مصدر ticks فعلي عند التشغيل، بلا سلطة على المشروع
Project Scheduler       = يجدول العمل المطلوب عند ProjectTime
```

### 22.2 فصل أربعة مفاهيم يجب ألا تختلط

| المفهوم | وظيفته | ما لا يملكه |
|---|---|---|
| `ProjectTime` | نطاق rational/integer يحدد موضع كل clip وkeyframe وautomation | لا يدق ولا يتقدم وحده |
| `ClockSource` | يعطي ticks مرتبطة بجهاز أو host monotonic clock | لا يعرف clips ولا revisions |
| `ProjectTransportAuthority` | يملك play/pause/seek/rate/epoch ويحوّل ticks إلى ProjectTime | لا يفك الفيديو ولا يرسم UI |
| `ProjectScheduler` | يقيّم المشروع ويصدر audio ranges وvideo/render deadlines | لا يغير ProjectTime من تلقاء نفسه |

الصوت لا يصبح حقيقة المشروع لأنه استُخدم كـClockSource؛ تمامًا كما لا تصبح ساعة المعالج مالكة للمشروع عند غياب الصوت.

### 22.3 المسار المرجح للسلطة

```text
UI / Agent / System Transport Command
                  │
                  ▼
       ReFusion Command Gateway
                  │ validate
                  ▼
     Project Transport Authority
   state + epoch + rate + time anchor
                  │
          reads ClockSource ticks
                  │
                  ▼
          Canonical ProjectTime
                  │
       ┌──────────┼───────────┬────────────┐
       ▼          ▼           ▼            ▼
  Audio Graph  Video Graph  Animation   Automation
       │          │           │            │
       └──────────┴───────────┴────────────┘
                  │
                  ▼
        Unified Project Evaluation
```

Timeline وCanvas وInspector تعرض `TransportSnapshot` و`ProjectRevision` مقبولتين. لا توجد UI timer تحدد الوقت، ولا video callback يدفع playhead، ولا AudioClip تعيد كتابة ProjectTime.

### 22.4 عقد Transport Epoch — `L + E`

يقترح تمثيل runtime غير الدائم بصورة بحثية مثل:

```text
TransportEpoch {
    transport_epoch_id
    state
    project_anchor_time: RationalTime
    clock_anchor_tick: ClockTick
    playback_rate: Rational
    clock_source_id
}

EvaluationGeneration {
    transport_epoch_id
    accepted_revision_id
}
```

أثناء `Running`:

```text
clock_elapsed = ClockSource.toRationalDuration(
                    Clock(now) - clock_anchor_tick)

ProjectTime(now) = project_anchor_time + clock_elapsed × playback_rate
```

هذه المعادلة تستخدم arithmetic checked ومحددًا، وليست `double seconds`. كل work item يوسم بزوج `(transport_epoch_id, accepted_revision_id)`: تغير epoch يلغي عمل seek/clock قديم، وتغير revision يلغي تقييمًا قديمًا من دون إعادة الساعة تلقائيًا.

`ProjectRevision` تبقى حقيقة التحرير الدائمة. أما `TransportEpoch/State` فهي حقيقة جلسة تشغيل مؤقتة؛ تقدم playhead لا ينشئ Project Revision كل frame.

### 22.5 كيف يبدأ التشغيل من دون دائرة اعتماد؟

قد يبدو أن Audio Graph يحتاج ProjectTime لإنتاج الصوت، بينما ProjectTime يأخذ نبضه من جهاز الصوت. الحل هو anchor + priming:

1. يستقبل Transport أمر Play عند `P0` ويخلق epoch جديدة.
2. يقيّم Audio Graph مسبقًا ويملأ output buffer بعينات مجال يبدأ عند `P0`.
3. يبدأ audio endpoint ويأخذ correlation بين device sample position وhost time.
4. بعد ثبات correlation، يثبت `clock_anchor_tick ↔ project_anchor_time P0`.
5. يتقدم ProjectTime بحسب العينات التي يستهلكها الجهاز، لا بحسب العينات التي جهزها المحرك فقط.
6. يجدول Project Scheduler الفيديو والأنيميشن والـautomation على ProjectTime المقاس والمتوقع.

إذن لا توجد دائرة منطقية: `P0` يرسخ البداية، والـdevice consumption يقيس التقدم بعدها.

بعض المسارات لا تعطي hardware timestamp مستقرة فور التشغيل أو بعد route change. عندها تدخل الجلسة حالة `Priming/ClockAcquiring` وتستخدم correlation مؤقتة معلنة؛ لا يجوز hard-switch يسبب قفزة في ProjectTime. سياسة lock/discipline والمهلة قبل fallback تحتاج E-031.

Apple يمثل اللحظة صوتيًا كـsample time وhost time في `AVAudioTime`. Windows يعرض device position مع performance-counter correlation في `IAudioClock::GetPosition`. Android يعرض frame position مع زمن monotonic مقدر في `AudioTrack.getTimestamp`. يجب تغليفها كلها داخل `ClockSourceAdapter` داخلي لا تتسرب أنواعه إلى بقية المحرك.

### 22.6 أي صوت هو الماستر في مشروع متعدد المسارات؟

لا يوجد audio clip ماستر، ولا track منفرد، ولا الفيديو الذي أُدرج أولًا. التسلسل هو:

```text
All audio clips/tracks/buses
             │
             ▼
      Project Audio Graph
             │
             ▼
       Master Output Bus
             │
             ▼
     Physical Audio Endpoint
             │
             ▼
      Device ClockSource ticks
```

جهاز الإخراج النهائي هو الذي يوفر النبض، بينما Transport Authority تبقى صاحبة mapping إلى ProjectTime. Plugin latency compensation وtrack delay compensation تحدث قبل Master Output مع الحفاظ على المجال الزمني المطلوب.

### 22.7 تحويل ProjectTime إلى زمن كل مصدر

عند زمن مشروع `P`، لا يبحث المحرك مباشرة عن frame رقم `P × FPS`. لكل clip دالة تحويل مستقلة:

```text
clip_local_time = P - clip_project_start
source_time = source_in + TimeMap(clip_local_time)
```

`TimeMap` قد تكون identity أو rate أو reverse أو piecewise time-remap. بعدها فقط يستخدم Source Media Time Contract من القسم 21 لتحويل `source_time` إلى:

- video PTS/duration الصحيحة.
- audio sample interval الصحيحة.
- effect/animation parameters عند ProjectTime نفسه.

وهكذا يحتفظ كل فيديو بزمن مصدره الحقيقي، مع وجوده في إحداثيات مشروع موحدة.

### 22.8 اختيار ClockSource حسب الوضع

| وضع الجلسة | ClockSource المرجح | الملاحظة |
|---|---|---|
| تشغيل أمامي بصوت وجهاز مستقر | Audio device/sample clock | الأفضل لمنع audio under/overrun وإبقاء الفيديو تابعًا للاستهلاك الحقيقي |
| مشروع بلا إخراج صوتي | High-resolution monotonic clock | مع vsync prediction للعرض فقط |
| Pause | ProjectTime ثابت داخل epoch | لا يشتق من wall clock |
| Seek/Scrub | طلبات target ProjectTime صريحة | لا يوجد free-running clock أثناء still evaluation |
| Reverse/Shuttle أو معدلات خاصة | يحتاج سياسة مستقلة؛ غالبًا monotonic transport مع audio strategy صريحة | لا يفترض أن audio endpoint يستطيع قيادة reverse semantics |
| Offline Export | لا ClockSource حقيقية | evaluator rateless يمر على timestamps/sample ranges الحتمية |
| External timecode لاحقًا | External disciplined source adapter | مؤجل ولا يمنح المصدر الخارجي سلطة تعديل المشروع |

يبقى اختيار clock policy بيد Transport Authority. لا تختاره UI ولا الـbackend منفردًا.

### 22.9 تشغيل الفيديو والصوت عند ProjectTime

Project Scheduler لا ينتظر حلول playhead كي يبدأ العمل؛ يجدول نوافذ مستقبلية bounded:

- Audio Graph تنتج sample ranges المطلوبة للـoutput device ahead of deadline.
- Video Scheduler يفك dependencies مسبقًا ثم يختار frame ذات PTS المطابقة لـProjectTime المتوقع عند vsync.
- Render Graph يقيّم transforms/effects/composition للـrevision المقبولة نفسها.
- إذا تأخر video frame، تطبق drop/hold policy ولا يتغير ProjectTime ولا يتمدد الصوت سرًا.
- إذا حدث audio underrun، تسجل حقيقة failure؛ سياسة pause/re-prime مقابل الاستمرار تحتاج تجربة ولا يجوز إخفاؤها بقفزة زمنية.

الـqueues اللازمة هنا bounded ومملوكة للمحرك. وجودها لا يجعلها ساعة ولا مصدر حقيقة.

### 22.10 Seek وPause وتبديل جهاز الصوت

كل discontinuity كبيرة تنشئ epoch جديدة:

```text
Seek / route change / device loss / incompatible rate change
                          │
                          ▼
            invalidate old epoch work
                          │
             flush/re-prime affected paths
                          │
                          ▼
             establish new clock anchor
```

متطلبات السلوك:

- لا يصل frame أو audio block من epoch قديمة إلى output.
- `ProjectTime` الهدف يأتي من Command مقبول، لا من موضع playhead الرسومي.
- route change تعيد correlation مع clock الجديدة.
- إذا أمكن seamless re-anchor، يبقى ProjectTime مستمرًا حسابيًا.
- إذا لم يمكن، تعلن session discontinuity أو pause/re-prime؛ لا تخفي gap.
- loss of hardware timestamp ينتج degraded-clock diagnostic وينقل إلى fallback معلن فقط وفق policy معتمدة لاحقًا.

### 22.11 التعديل أثناء التشغيل

عند قبول Project Command أثناء playback:

- ينتج `ProjectRevision` جديدة ذرية.
- يختار scheduler حدًا آمنًا للانتقال إلى revision الجديدة.
- يثبت `accepted_revision_id` داخل `EvaluationGeneration` وtransport snapshot المستخدمتين للتقييم.
- لا تقفز الساعة لمجرد تغيير لون أو effect.
- التعديل الذي يغير بنية الزمن نفسها، مثل حذف مجال عند playhead أو تغيير time map، يحتاج transition policy وepoch جديدة عند الضرورة.

لا ينبغي أن يرى Canvas مزيجًا من audio من revision قديمة وvideo من revision جديدة.

### 22.12 Realtime Preview مقابل Offline Export

المصدر المشترك للحقيقة هو `Project Evaluator(ProjectRevision, ProjectTime)`، وليس clock:

```text
                    Project Evaluator
                     /             \
                    /               \
Realtime Transport Clock         Offline Timestamp Iterator
audio/monotonic sourced          deterministic + rateless
```

Realtime يقرر **متى** يطلب الزمن وقد يفوّت deadline. Offline يقرر تسلسل الأزمنة من output specification ولا يفوّت frame بسبب wall time. كلاهما يجب أن يختار clip/source PTS وaudio samples والـautomation نفسها عند ProjectTime نفسها.

### 22.13 المسارات المستبعدة مبدئيًا

- AudioClip أو VideoClip كمالك لزمن المشروع.
- UI timer أو animation loop كساعة للـTimeline.
- جعل وصول decoder callback يحرك playhead.
- ساعة مستقلة لكل track أو layer.
- اعتماد wall-clock time مثل تاريخ النظام.
- تقدم ProjectTime بعدد frames المفكوكة أو المعروضة.
- إنشاء Project Revision لكل tick أو frame.
- السماح لكل backend بتعريف معنى seek/rate مختلف.
- تغيير clock source بلا epoch/re-anchor/diagnostic.
- استخدام صوت المصدر قبل الـmix كـmaster بدل final output endpoint.

### 22.14 التجارب المطلوبة

#### E-028 — Project Transport Mapping

- تحقق exact من معادلة anchor عبر play/pause/resume وrates متعددة.
- تشغيل طويل وقياس drift بين ProjectTime وdevice sample position.
- إثبات أن playhead snapshot قارئ فقط وأن UI timer لا يغير الزمن.

#### E-029 — Multitrack Unified-Time Sync

- عدة videos وaudio tracks وanimations وautomation عند landmarks مشتركة.
- plugin latency وparallel buses وmute/solo.
- إثبات أنها تُقيّم على ProjectTime واحدة ولا يصبح أي clip ماستر.

#### E-030 — Epoch and Discontinuity Safety

- rapid seek/scrub.
- audio route/device change.
- pause أثناء decoder work.
- accepted revision أثناء playback.
- إثبات عدم تسرب frame/audio block من epoch أو revision قديمة.

#### E-031 — Clock Policy Matrix

- audio present/absent/muted/unavailable.
- Bluetooth وHDMI وspeaker/headphones.
- forward/rate change/reverse/shuttle.
- timestamp warmup/unavailability وfallback diagnostics.

#### E-032 — Realtime/Offline Project-Time Parity

عند مجموعة ProjectTime ticks محددة، تقارن preview capture وoffline render في:

- source PTS المختارة لكل clip.
- audio source/output sample ranges.
- animation وautomation values.
- accepted revision ID.

الاختلاف المسموح في realtime هو presentation deadline/drop المعلن، لا اختلاف معنى الزمن أو المحتوى المطلوب.

### 22.15 الحكم المؤقت لهذه الجولة

> `ProjectTime` هي الحقيقة الزمنية الموحدة، و`ReFusion Project Transport Authority` داخل Engine Session هي مالك التشغيل الوحيد. Audio endpoint clock يزودها بالنبض أثناء التشغيل الأمامي مع صوت، لكنه لا يملك المشروع. Transport تربط النبض بـProjectTime عبر epoch/anchor، وProject Scheduler يقيّم جميع الفيديوهات والصوت والأنيميشن والمؤثرات على الزمن نفسه. عند غياب الصوت تستخدم ClockSource أخرى معلنة، وعند export لا توجد ساعة realtime أصلًا بل evaluator حتمي rateless.

لا يصبح هذا الحكم قرارًا حتى نجاح E-028 إلى E-032 وحسم سياسات mute وunderrun وroute change وreverse playback.

---

## 23. غربلة الانتقال من النواة إلى منتج v1 قابل للبيع والترقية

> **حالة هذا القسم: غربلة Draft 0.6 غير معيارية وغير قابلة للاستخدام كـMaster Plan أو مواصفة تنفيذ.** المراحل والحدود والأسماء التالية مرشحة للمراجعة، والغرض منها منع بناء محرك طويل بلا تطبيق منشور.

### 23.1 إعادة صياغة المشكلة

الخطر ليس نقص الطموح، بل بناء عشرات الأنظمة أفقيًا ثم اكتشاف أن المنتج لا يملك:

- Installer يعمل على جهاز مستخدم نظيف.
- Creator Loop كاملة من import إلى export.
- حفظًا واستردادًا وترقية آمنة للمشروع.
- parity بين UI والـAgent.
- مسار توقيع وتحديث وتشخيص ودفع.
- نطاقًا يمكن دعمه وبيعه.

المسار المرجح هو:

> **Walking Product Skeleton + Release Spine + Narrow Paid Vertical Slice**

أي أن التطبيق القابل للتثبيت يبدأ مبكرًا، وكل Gate لاحقة تضيف قدرة عمودية كاملة إليه. لا توجد مرحلة اسمها «نبني المحرك كله» ثم مرحلة أخرى منفصلة اسمها «نصنع المنتج».

### 23.2 معنى «احترافي بلا مشاكل»

لا يمكن ضمان برنامج بلا عيب 100%. الصياغة المهنية القابلة للإثبات هي:

- عقود واضحة يمكن اختبارها.
- نطاق مغلق لكل إصدار.
- Exit Criteria رقمية ومقبولة قبل الانتقال.
- crash containment وrecovery وrollback.
- telemetry تحترم الخصوصية وتكشف الواقع.
- دعم ملفات وأجهزة معلن بدل ادعاء دعم غير محدود.
- إصدار متكرر من artifact واحدة موقعة ومختبرة.

الهدف ليس غياب المشكلات نظريًا، بل منع المشكلة الواحدة من إتلاف المشروع، ومنع نشر release لا نعرف حالتها.

### 23.3 Creator Loop المرشحة للنسخة المدفوعة الأولى

ينبغي أن تبيع v1 نتيجة مكتملة واحدة:

```text
Install
  → Create/Open Project
  → Import Video/Image/Audio
  → Edit via UI or Agent
  → Add Text/Shape + Animation + Mask + FX
  → Preview
  → Save/Close/Reopen
  → Export a shareable video
  → Update/Recover without losing the project
```

التموضع التجاري المرشح للغربلة هو **Agent-native short-form video and motion creation**: إنتاج فيديو قصير أو إعلان/عنوان متحرك عالي الجودة، لا منافسة كل وظائف Premiere وAfter Effects وResolve في الإصدار الأول.

### 23.4 نطاق Desktop v1 المرشح — `L + E`

#### داخل الشريحة الرأسية

- macOS وWindows من Gate التطبيق الأولى؛ لا يتم تأجيل port إلى نهاية المشروع.
- architecture أولية محتملة: Apple Silicon وWindows x64؛ Intel Mac وWindows ARM قرارات matrix مستقلة.
- SDR/Rec.709 و1080p كحد أول مرشح، مع عدم ادعاء HDR/4K قبل القياس.
- Media Matrix ضيقة وموثقة؛ مرشح أولي: H.264/AAC داخل MP4، PNG/JPEG وWAV، رهينة مراجعة codec/licensing/hardware paths.
- `Video`, `Audio`, `Image`, `Text`, `Shape`, `Group`، و`Adjustment` محدودة لإثبات scoped processing.
- trim/split/move/active range، Transform2D، crop، opacity وblend المتوافق.
- Keyframes من الأنواع Step/Linear/Cubic Bezier عبر Property System واحدة.
- Mask path مع add/subtract/invert/feather ضمن عقد واحد.
- مجموعة FX مرئية صغيرة: Blur وColor Controls وDrop Shadow مثلًا؛ العدد ليس هدفًا.
- waveform حقيقية وGain/Pan/Fade، مع Audio Studio shell تقرأ clip نفسها؛ الاستعادة المتقدمة تدخل فقط إذا نجحت بوابة مستقلة.
- Undo/Redo، autosave/journal، crash recovery وmissing-media relink.
- UI وAgent ينفذان الأوامر العامة نفسها.
- Preview/Export بالدلالة نفسها.
- تصدير واحد موثوق يمكن للمستخدم استخدامه فعليًا.

#### `Not V1` مبدئي

- HDR/RAW/multicam وcodec matrix واسعة.
- 3D/camera/light/particles/simulation.
- motion tracking وrotoscoping/optical-flow suite كاملة.
- collaboration cloud متعدد المستخدمين.
- Expressions عامة غير معزولة.
- Node Editor عام.
- third-party native plugins داخل العملية.
- Marketplace عام.
- mobile Studio كاملة.
- عشرات FX أو استنساخ واجهة تطبيقات أخرى.

التأجيل لا يعني إزالة العقود اللازمة للتوسع؛ يعني عدم تنفيذ الأسطح والميزات قبل أن تمولها Creator Loop مثبتة.

### 23.5 التوحيد المهني للـLayers

المسار المرجح ليس inheritance tree بعقود منفصلة مثل `VideoLayer`, `TextLayer`, `ImageLayer` لكل منها animation/effects خاصة. وليس BaseLayer ضخمة تحتوي خصائص وهمية لكل الأنواع.

المرشح:

> **Unified Layer Entity + Descriptors/Capabilities + Typed Ports + One Property System**

```text
LayerNode
├── stable id
├── kind: schema_id + schema_version
├── parent/order/enabled/locked
├── active_range + time_map
├── typed property states
├── relationships
└── effect/mask attachments
```

`kind` يشير إلى Descriptor مسجلة في Engine Capability Registry. الـDescriptor تعلن ما يستطيع النوع فعله والمنافذ التي ينتجها:

| النوع | قدرات مرشحة | المخرج الدلالي |
|---|---|---|
| Video | TemporalSource، VisualProducer، Transform2D | `VisualSurface` |
| Image | VisualProducer، Transform2D | `VisualSurface` |
| Text | TextLayout، VisualProducer، Transform2D | `VisualSurface` |
| Shape/Vector | VectorGeometry، VisualProducer/MaskProducer | `VisualSurface` أو `CoverageMask` |
| Audio | TemporalSource، AudioProducer، AudioMixable | `AudioBus` |
| Group | Container، VisualCompositor وربما AudioMixer | typed outputs مستقلة |
| Adjustment | ScopedProcessor | `VisualSurface → VisualSurface` |

التوحيد يعني هوية وزمنًا وأوامر وخصائص وحفظًا وفحصًا موحدة. لا يعني إجبار الصوت على Transform بصري، أو تطبيق Gaussian Blur على AudioBus. كل Effect تعمل على **أي Layer ذات port متوافق**، وهذا هو المعنى المهني لعبارة «FX لكل Layer».

### 23.6 Property وAnimation System واحدة

كل قيمة قابلة للتحرير تستخدم schema واحدة، سواء كانت Position أو Gain أو Blur Radius:

```text
PropertySchema
├── stable_id + schema_version
├── value_type + unit
├── default + constraints
├── animatable + interpolation modes
├── expression policy
├── coordinate/color/time semantics
└── Inspector hints

PropertyState
├── authored_value
├── optional animation_curve
└── optional expression_binding
```

المعرّف ثابت وغير مترجم؛ label UI منفصل. الأنواع والوحدات وفضاءات الإحداثيات صريحة. قيمة الزمن تستخدم `RationalTime`، ولا توجد animation evaluator خاصة لكل Layer.

المسار المرشح:

```text
Authored Value
      │
      ▼
Animation Curve at ProjectTime
      │
      ▼
Optional deterministic expression
      │
      ▼
Resolved Typed Value
```

تحجز v1 مكان Expression في العقد، لكن تؤجل runtime عامة حتى يثبت sandbox وdependency graph وcycle detection والحتمية. Keyframes موثوقة أهم للمنتج الأول من لغة تنفيذ واسعة.

### 23.7 Masks وEffects كعقد typed

```text
EffectDescriptor
├── stable_id + version
├── typed input/output ports
├── PropertySchemas
├── allowed stages
├── temporal radius/latency
├── color/alpha contract
└── supported backends/platforms

EffectInstance
├── instance_id
├── descriptor_id/version
├── property states
└── connections
```

أمثلة:

```text
GaussianBlur : VisualSurface → VisualSurface
AudioEQ      : AudioBus      → AudioBus
VectorMask   : VectorPath    → CoverageMask
```

الـMask ليست field سرية لكل Layer، بل `CoverageMask` و`MaskBinding` موحدان مع operation وinvert وfeather وtransform space.

ترتيب أولي يحتاج حسمًا بالتجربة:

```text
Content Producer
  → Local Effect Chain
  → Mask/Matte
  → Transform
  → Opacity/Blend
  → Parent Composite
  → Scoped Adjustment
```

Group وAdjustment يجب أن تتحولا إلى subgraph وscope input صريحين عند compile؛ لا يبقى معنى «ما تحتها» سلوكًا سحريًا لا يفهمه Agent.

### 23.8 المستويات الثلاثة للرندر

```text
Canonical Project Semantic Graph
               │
               ▼
Typed Evaluation Graph at Revision + ProjectTime
               │
               ▼
Backend Execution Plan
Metal / D3D / Skia / GPU compute / Audio backend
```

- Project Model لا يعرف `SkImage*` أو Metal/D3D handles أو Qt objects.
- Semantic Graph Compiler يتحقق من ports والدورات والـcapabilities.
- Visual وAudio graphs منفصلتان تنفيذًا ومتزامنتان بـProjectTime.
- Preview وExport تستعملان evaluator واحدة؛ تختلف scheduling/quality فقط.
- cache/invalidation مشتقة من Revision/Node/time/output contract.
- Skia وwgpu/native backends منفذات، لا مصدر Schema أو حقيقة مشروع.

### 23.9 Capability Spine: تعريف الإنجاز لكل ميزة

أي Layer أو Effect أو Tool أو Property لا تعتبر مكتملة حتى تعبر السلسلة كلها:

```text
Descriptor/Capability
  → Typed Command
  → Validation
  → Accepted Revision
  → Serialization + Migration
  → Inspector/Timeline
  → Canvas Preview
  → Export
  → Undo/Redo + Replay
  → Agent Introspection
  → macOS/Windows Tests
  → Diagnostics/Docs
```

هذه هي `Capability Definition of Done`. تمنع:

- Feature موجودة في UI فقط.
- أمر Agent لا يظهر في التطبيق.
- Effect يعمل في Preview ولا يعمل في Export.
- Layer تحفظ بيانات غير قابلة للترقية.
- capability معلنة بلا backend أو Diagnostic.

### 23.10 Agent Surface وEngine Skill

المصدر الحقيقي لفهم Agent ليس مستندًا يدويًا ثابتًا، بل snapshot مولدة من Registry نفسها:

```text
Engine Capability Registry
├── Layer Descriptors
├── Property Schemas
├── Effect/Mask Descriptors
├── Command Schemas
├── platform availability
└── version/diagnostics
             │
             ├── Inspector generation
             ├── Agent catalog/introspection
             ├── SDK documentation
             └── ReFusion Engine Skill bundle
```

أوامر عامة مرشحة:

```text
CreateLayer(kind_id, initial_properties)
SetProperty(property_ref, typed_value)
UpsertKeyframe(property_ref, time, value, interpolation)
AttachEffect(target_id, descriptor_id, stage, index)
BindMask(target_id, mask_id, operation)
Reparent(node_id, parent_id, order)
SetActiveRange(node_id, range)
```

كل Command تحمل `expected_revision`, `idempotency_key` وحد transaction ذرية. ويرجع المحرك `AcceptedRevision + SemanticDiff + Diagnostics`.

الـSkill توضح طريقة استخدام المحرك، لكنها ليست مصدر Capability مستقلًا. عند إضافة Effect جديدة، يكتشفها Agent من schema المسجلة، ولا نحتاج command family خاصة لكل Layer.

### 23.11 حدود الحزم المفاهيمية

هيكل مستقبلي مرشح، لا أسماء مجلدات معتمدة:

```text
ReFusion Studio UI
        │ Commands / immutable snapshots
        ▼
Command + Project Core
        │ accepted revision
        ├── Property/Animation/Capability Registry
        ├── Semantic Graph Compiler
        ├── Project Transport/Scheduler
        ├── Media + Audio Engines
        └── Render/Export Engine
                 │
          Platform Adapters
       macOS/Metal/CoreAudio/VT
       Windows/D3D/WASAPI/MF

Extension Supervisor ── isolated workers/hosts later
Release/Updater/Licensing/Diagnostics ── product services
```

تقسيم source tree استكشافي يعكس هذه الحدود، لا أسماء نهائية:

```text
engine/       project, command, schema, time, evaluator
runtime/      render, media, audio, export
platform/     macos, windows, ios, android adapters
studio/       Qt/QML application and view models
extensions/   registry, supervisor, wasm worker, native host
sdk/          schemas, generated bindings, samples, conformance kit
product/      packaging, updater, licensing, diagnostics
tests/        fixtures, corpora, golden, migration, release tests
```

قواعد الحدود:

- UI لا ترتبط بdecoder أو GPU frame types أو plugin binaries.
- Core لا يرتبط بـQt types.
- Project لا يخزن backend objects.
- Platform adapters لا تعيد تعريف semantics.
- built-in capabilities تسجل عبر Registry نفسها التي ستخدم SDK لاحقًا.
- Release, updater, recovery وdiagnostics ليست أعمالًا جانبية؛ هي أجزاء منتج.

### 23.12 Gates المرشحة للخطة المستقبلية

| Gate | الناتج القابل للاستخدام | Exit Criteria الأساسية |
|---|---|---|
| `G0 Product Contract` | persona/use-case، reference projects، Media Matrix، Not‑V1، budgets | اختبار قبول مكتوب من install إلى export، ومراجعة licensing مبكرة |
| `G1 Walking Release Skeleton` | تطبيق macOS/Windows يفتح وينشئ ويحفظ مشروعًا ويعرض/renders fixture | حزم موقعة/قابلة للتثبيت تعمل على أجهزة نظيفة بلا toolchain |
| `G2 Creator Loop` | import، timeline، Video/Image/Text/Audio، preview، save/reopen، export | مستخدم داخلي ينتج reference video بالكامل من التطبيق |
| `G3 Unified Authoring` | Shape/Group/Adjustment، Property/Keyframe/Mask/FX الموحدة | نفس FX/animation المتوافقة تعمل على visual layer types بلا contracts متفرعة |
| `G4 Agent Parity` | discovery، validate/dry-run، commit، undo/replay | UI وAgent للأمر نفسه ينتجان semantic revision digest نفسه |
| `G5 Product Hardening` | autosave/recovery/relink/diagnostics/performance/unsupported UX | fault/soak/corpus tests تمر ولا يوجد فقد مشروع |
| `G6 Paid Founder Beta` | signing/notarization/update/onboarding/license/payment/support | مستخدم خارجي يدفع ويثبت ويفعل وينتج ويصدر ويحدّث بلا تدخل المطور |
| `G7 Stable v1` | release مدفوع موثق مع support/rollback | لا Blockers، وcrash/export/update metrics تحقق budgets المعتمدة |

من `G1` فصاعدًا، كل Gate تنتج Installer على المنصتين. Demo داخل IDE ليست artifact إنتاجية.

### 23.13 كيف تُقسم الأعمال داخل كل Gate؟

لا تنشأ «مرحلة Backend» ثم «مرحلة UI» ثم «مرحلة Agent». لكل Gate خمسة lanes تتحرك معًا:

1. **Core Contracts:** Project/Command/Schema/Time/Migration.
2. **Media and Evaluation:** decode/audio/render/export/performance.
3. **Studio Experience:** Timeline/Inspector/Canvas/onboarding/accessibility.
4. **Agent and SDK Surface:** introspection/commands/diagnostics/docs.
5. **Product Delivery:** packaging/signing/update/telemetry/license/support.

لكل work item داخل Gate:

- user outcome.
- العقد المتأثرة.
- dependencies والمالك.
- acceptance fixtures.
- platform matrix.
- security/licensing impact.
- performance budget.
- rollback/failure behavior.
- Exit evidence.

بهذا لا تنفصل الخطوات الهندسية؛ كل milestone تقطع النظام عموديًا.

### 23.14 Release Spine من أول تطبيق

#### لكل تغيير/PR

- build macOS وWindows.
- unit/property tests وcommand replay.
- schema migration fixtures.
- shader compilation لكل backend مستهدف.
- golden visual/audio comparisons ضمن tolerance موثقة.
- dependency/license/vulnerability scan.
- منع merge إذا انكسر installable trunk أو reference project.

#### اختبارات دورية على أجهزة GPU حقيقية

- media conformance corpus.
- seek/playback/export soak.
- device loss وaudio route change.
- memory/GPU budget.
- font/text/color cross-platform parity.
- فتح مشاريع من إصدارات سابقة.

#### Release Candidate

1. build من commit/tag وdependency lock ثابتين.
2. توليد symbols وSBOM وbuild provenance.
3. توقيع artifact وupdater metadata.
4. install/launch/export/update/rollback على أجهزة نظيفة.
5. فحص migration على نسخ projects محفوظة.
6. ترقية **artifact نفسها** من internal إلى beta ثم stable حيث تسمح القناة؛ وإذا أعاد Store التوقيع أو التغليف، ترتبط النتيجة بـcontent digest وprovenance للبناء نفسه.

القنوات المرشحة:

```text
Developer → Internal → Private Alpha → Paid Founder Beta → Stable
```

### 23.15 نشر macOS وWindows

#### macOS — shortlist

- `macdeployqt` لتجميع Qt runtime/plugins، لا كبديل للتدقيق الكامل.
- Developer ID وتوقيع nested code.
- Hardened Runtime.
- notarization بواسطة `notarytool` وstapling/verification.
- DMG أو PKG حسب تجربة التثبيت.
- updater موقّع وذري؛ Sparkle 2 أو Qt Installer Framework مرشحان للغربلة.

Apple توضح أن notarization تفحص التوقيع والمحتوى، وأن Hardened Runtime يؤثر في plug-ins أيضًا. لذلك يجب إجراء packaging/plugin-host spike مبكر.

#### Windows — shortlist

- `windeployqt` لتجميع Qt dependencies.
- MSIX + `.appinstaller` + signing كمرشح أول.
- Microsoft Store قناة لاحقة/موازية، لا اعتماد وحيد قبل اختبار السوق.
- Signed WiX/MSI/EXE مع updater كبديل إذا أعاقت MSIX مواقع media/plugin/cache أو workflows المهنية.
- timestamped signing وclean-machine install tests.

Windows يفرض توقيع MSIX بثقة صالحة. لا يُحسم MSIX مقابل WiX بالنظر؛ يجب إثبات update/repair/plugin directories/file associations على أجهزة فعلية.

### 23.16 Licensing Gate قبل الالتزام بالـUI stack

Qt dual-licensed. المساران المرشحان:

- Commercial Qt وفق عقد مناسب لكل المطورين/التوزيع.
- LGPL-compliant dynamic distribution مع كل الالتزامات وإتاحة relinking/source notices حيث تنطبق.

بعض Qt modules GPL-only في open-source offering، والمتاجر/DRM قد تضيف تعارضات. لذلك:

> لا يُعامل «Qt مجاني» أو «سنشتريه لاحقًا» كافتراض هندسي.

يجب تسجيل module/license inventory، linking mode، store/direct-distribution policy ومراجعة قانونية قبل تثبيت UI stack في Master Plan. وينطبق gate مماثل على FFmpeg configuration/codecs وأي AI models/assets/fonts.

### 23.17 Diagnostics وقياسات المنتج

المطلوب ليس جمع محتوى المستخدم. القياسات المرشحة:

- crash-free sessions وsymbolicated crash groups بحسب build ID.
- import/export/update/activation success.
- first-frame وseek/export performance budgets.
- GPU/backend/driver/codec class من دون أسماء ملفات.
- Agent command validation/commit success وأسباب الرفض.
- save/reopen/recovery success.
- time-to-first-successful-export.

قواعد الخصوصية:

- opt-in/notice واضح حسب المتطلبات القانونية.
- لا project text، media frames، filenames أو prompts خام افتراضيًا.
- Diagnostics Bundle محلية قابلة للمراجعة قبل الإرسال.
- dSYM/PDB/symbols مرتبطة بـartifact الدقيقة وتحفظ آمنًا.

Sentry Native/Crashpad وخدمات مشابهة مرشحون، لا قرار نهائي.

### 23.18 حلقة العائد الأولي

الهدف من Beta المدفوعة ليس تمويل كل الرؤية فورًا، بل إثبات أن مستخدمًا يدفع مقابل Creator Loop الحالية.

مرشح بسيط للغربلة:

- Trial/Free محدودة التصدير أو بعلامة واضحة.
- Founder/Pro plan واحدة بدل tiers كثيرة.
- Agent cloud usage منفصلة أو quota واضحة إذا كانت لها تكلفة تشغيل.
- signed entitlement lease مع offline grace.
- التخزين الآمن في Keychain/Credential Manager.
- انتهاء الاشتراك لا يمنع فتح مشروع المستخدم ولا يحذف بياناته؛ يقيد capability مدفوعة بصورة معلنة.

Direct checkout/Stripe أو Store billing أو Merchant of Record تحتاج مقارنة ضرائب/بلدان/سياسات مستقلة. لا يوضع secret تجاري داخل التطبيق، والسيرفر هو حقيقة entitlement إذا اختير نموذج خدمة.

بوابة التوسع بعد Beta يجب أن تستخدم أدلة مثل:

- عدد المستخدمين الذين وصلوا إلى first export.
- retention واستخدام Agent الفعلي.
- أكثر failures تكلفة.
- willingness to pay ودعم العملاء.
- نسبة الإيراد إلى تكلفة cloud/support.

### 23.19 القرار المرحلي لنظام Plugins

> **v1 تكون Plugin-ready، وليست Public-Plugin-SDK-ready.**

يدخل v1:

- Unified Extension Registry داخلية.
- Contribution descriptors للأدوات وFX/importers المدمجة.
- typed schemas يقرأها Inspector والـAgent.
- declarative effect graphs/presets فوق nodes موثوقة.
- package/manifest/state versioning داخلي.
- project references تحفظ plugin/contribution IDs وإصداراتها.
- `UnresolvedNode` يحفظ بيانات قدرة مفقودة بلا حذف.
- Plugin Supervisor skeleton وfault-injection prototype.

لا يدخل v1:

- تحميل DLL/dylib طرف ثالث داخل Main App.
- تجميد C++ ABI عام.
- Marketplace عام.
- custom UI panels اعتباطية.
- third-party realtime GPU/audio paths.
- arbitrary scripting داخل UI/Render/Command processes.

السبب: Public SDK المبكرة تحول تفاصيل غير ناضجة إلى التزام توافق دائم، وقد تجعل أي plugin يسقط التطبيق أو يفسد المشروع.

### 23.20 Extension Contract الموحدة — `L + E`

جميع runtimes المستقبلية تسجل Contribution نفسها:

```text
Contribution
├── CommandProvider
├── EffectNode
├── Importer
├── Tool
└── InspectorContribution
```

ويعلن كل منها:

- stable contribution ID وstate schema version.
- typed input/output/parameter schemas.
- capabilities والصلاحيات المطلوبة.
- realtime مقابل asynchronous.
- determinism policy.
- latency/temporal radius.
- color/alpha/time contracts.
- platform وcontract compatibility range.

الـPlugin لا يستلم raw Project/Layer pointers. CommandProvider يقرأ snapshot محدودة ويرجع `CommandProposal`; Command Engine وحدها تنتج Accepted Revision. Importer يأخذ brokered file handle لا مسار filesystem حرًا.

### 23.21 مسارات SDK اللاحقة

#### v1.x — Portable/Safe Preview

- Wasm Component/WIT داخل Worker process معزول.
- Command providers/tools/metadata analyzers أولًا.
- default-deny filesystem/network/clock/random.
- memory/fuel/deadline limits.
- curated signed catalog، لا Marketplace مفتوح.
- لا GPU/audio realtime في هذا المسار قبل تجارب منفصلة.

#### v2 Desktop — Native Performance Track

- C++ SDK مريح كـwrapper.
- الحد الثنائي الحقيقي: stable versioned C ABI.
- التنفيذ داخل `ReFusionPluginHost` منفصل لكل package.
- opaque handles وfixed-width types و`ptr + length`.
- لا STL/RTTI/exceptions/class layout أو ownership مشتركة عبر الحد.
- RPC control plane؛ GPU shared-resource data plane لاحقًا فقط.
- importers وCPU FX قبل shared-GPU FX.
- OpenFX bridge محتمل داخل host، لا Core SDK عام.

#### v2.x وما بعدها

- certified shared-GPU effects.
- isolated custom UI.
- realtime audio host بعقد deadlines خاص.
- public marketplace مع signing/revocation/update framework.

مسار التأليف بواسطة Codex أو cloud لا يتجاوز الثقة:

```text
Codex/Developer authors SDK project
          → isolated build
          → conformance + fuzz + performance tests
          → developer/publisher signature
          → package review/channel policy
          → installation in isolated host
```

الكود المولد يعامل ككود طرف ثالث غير موثوق. Codex لا يحصل على marketplace root key، ولا يفعّل package فاشلة، ولا يكتب Project Revision مباشرة.

قاعدة الأمان:

> لا third-party code داخل UI Process أو Command Engine أو Render Engine الأساسي.

حتى Wasm يوضع داخل Worker مستقل كدفاع إضافي. `project.refusion.cpp` لا يُحمّل ككود غير موثوق داخل التطبيق المنتج؛ يمكن أن يصبح مدخل Build/SDK في Developer Mode أو package معزولة وموقعة لاحقًا.

### 23.22 Signing وPinning وترقية Plugins

هوية package المرشحة:

```text
plugin_id
publisher_key_id
version
artifact_digest
contract_range
contributions
capabilities
target_triples
payload_hashes
signature
```

- نفس version لا تشير إلى digest مختلف.
- native payload يحمل OS signing أيضًا.
- كل مشروع يثبت plugin ID + publisher + artifact digest + state schema.
- update تثبت side-by-side ثم تفعل ذريًا.
- capability escalation تحتاج موافقة جديدة.
- plugin update لا تغير output مشروع قديم بلا migration command صريح.
- Marketplace عامة لاحقًا تحتاج حماية rollback/freeze/revocation بمستوى TUF أو ما يعادله.

على macOS يبقى أي entitlement لاستضافة مكتبات خارجية محصورًا في Plugin Host. وعلى Windows، MSIX full-trust ليس sandbox؛ يجب غربلة AppContainer/Win32 isolation أو حد مكافئ.

### 23.23 mobile من دون جرّه إلى المسار الحرج

المسار المرجح:

- Desktop v1 هي release target.
- core/schema/command/property crates أو modules تبقى portable.
- compile-canary وrender/media probe دورية على iOS simulator/device وAndroid NDK/device.
- لا mobile UI كاملة قبل إثبات desktop product/revenue وقياس الحاجة.
- mobile تستخدم project/schema نفسها، لكن platform adapters وinteraction UI مختلفان.

نظام plugins لا يمكن أن يكون binary-identical:

- Apple تقيد تنزيل/تنفيذ كود يغير وظائف app.
- Google Play يقيد تنزيل `.so`/DEX/JAR خارج Play.
- mobile تبدأ بـpresets/graphs/assets/built-in modules أو cloud async وفق السياسات.
- التنفيذي first-party يوزع عبر قنوات المتجر المعتمدة.

إذن cross-platform يعني **دلالة مشروع مشتركة**، لا فرض طريقة توزيع plugins desktop على mobile.

### 23.24 Upgrade Governance بعد v1

- project format envelope version مستقل عن layer/effect/plugin schema versions.
- migrations حتمية ومتسلسلة ومختبرة بfixtures دائمة.
- حفظ ذري وjournal/recovery منفصلان عن caches.
- unknown extension data تمر round-trip بلا فقد.
- engine يفتح آخر N project versions حسب policy معلنة.
- لا migration destructive بلا backup وrollback path.
- feature flags لا تغير معنى مشروع محفوظ بلا versioning.
- deprecation تمر بمراحل: announce → warning → migration tool → removal في major boundary.
- release trains صغيرة؛ لا branch طويل يعيد دمج سنة من التغييرات.
- same artifact promotion بين channels.

كل تحديث جديد يجب أن يجيب:

1. ما capability الجديدة المسجلة؟
2. ما أثرها في project schema؟
3. كيف يكتشفها UI والـAgent؟
4. ما preview/export implementations؟
5. ما migration/rollback؟
6. ما platform matrix؟
7. ما licensing/security/performance evidence؟

### 23.25 شكل الـMaster Plan عندما يُكتب

ينبغي أن يكون Outcome/Gate-driven، لا قائمة subsystems. الهيكل المرشح:

1. **Product Contract:** persona، Creator Loop، reference projects، In/Not‑V1، pricing hypothesis.
2. **Non-negotiable Invariants:** command/revision/time/layer/property/effect/plugin/security contracts.
3. **ADRs and System Boundaries:** ownership، public/internal APIs، platform adapters.
4. **Gate Map G0–G7:** dependency graph، deliverables، exit evidence، release artifact.
5. **Conformance Program:** project/media/time/layer/agent/render/export/migration corpora.
6. **Release and Operations:** CI، signing، packaging، update، rollback، telemetry، incident response.
7. **Commercial Launch:** trial/entitlement/payment/privacy/support/onboarding.
8. **SDK and Upgrade Roadmap:** internal registry، Wasm preview، native host، mobile policy، deprecation.
9. **Risk Register:** owner، trigger، mitigation، kill criterion.
10. **Decision Log:** ما تم اعتماده، وما بقي مفتوحًا، وآخر موعد للحسم.

قالب كل Gate:

```text
Outcome
User scenario
Included / Excluded
Contracts touched
Dependencies
Deliverables
Platform matrix
Security/licensing review
Performance/reliability budgets
Acceptance fixtures and tests
Signed installable artifact
Rollback/failure behavior
Exit decision
```

### 23.26 قواعد منع الغوص غير المنتج

- كل مهمة ترتبط بخطوة في Creator Loop وGate محددة.
- كل بحث طويل يتحول إلى executable spike بمدة/مخرج/kill criterion.
- لا subsystem جديد قبل عبور vertical slice الحالية.
- لا Layer جديدة قبل إكمال Capability Definition of Done للأنواع الحالية.
- لا Effect جديدة قبل preview/export/save/agent/platform parity للـFX الحالية.
- لا public API قبل وجود مستهلكين داخليين اثنين على الأقل واختبار versioning.
- لا تحسين أداء بلا trace وfixture وbudget.
- لا mobile feature يزاحم desktop release قبل بوابة واضحة.
- فرع رئيسي قابل للبناء والتغليف خلف feature flags.
- Parking Lot يحتوي سبب التأجيل وشرط إعادة الفتح؛ ليس قائمة أمنيات خفية.
- المعيار ليس عدد classes أو commits، بل reference projects التي يستطيع مستخدم خارجي إنتاجها وإعادة فتحها وتصديرها.

### 23.27 تجارب وبوابات الإثبات الجديدة

#### E-033 — Signed Walking Release Skeleton

تثبيت وتشغيل وإزالة نسخة macOS وWindows على أجهزة نظيفة، مع create/save/reopen وrender fixture وتحقق التوقيع.

#### E-034 — Reference Creator Loop

إنتاج ثلاثة reference projects من داخل التطبيق: video+animated text، image/shape motion، وmask+FX+audio؛ ثم save/reopen/export.

#### E-035 — Unified Layer Descriptor

إضافة Prototype Layer عبر Descriptor/Producer من دون switch جديد في Inspector أو Agent command family.

#### E-036 — Property/Animation/Mask/FX Conformance

نفس Transform/keyframes/Blur/Mask على Video/Image/Text/Shape؛ رفض typed ومفهوم عند تطبيق visual FX على Audio.

#### E-037 — UI/Agent Revision Parity

نفس النتيجة عبر UI وAgent تنتج semantic state/revision digest نفسها، مع stale revision وidempotent retry وUndo/Redo.

#### E-038 — Persistence and Migration Safety

فتح مشاريع N-2/N-1، migration، forced crash، missing plugin/UnresolvedNode وround-trip بلا فقد.

#### E-039 — Preview/Export Cross-Platform Parity

Golden frames/audio ranges/property values على macOS وWindows مع tolerances معلنة وaccepted revision نفسها.

#### E-040 — Media and Performance Matrix

Reference codecs/devices/resolutions مع playback/seek/export/thermal/memory budgets وfail-closed unsupported UX.

#### E-041 — Fault/Recovery and Diagnostics

100 forced app terminations، device loss، audio route changes، export failure؛ recovery ناجحة وdiagnostics symbolicated بلا تسريب محتوى.

#### E-042 — Install/Update/Rollback

ترقية N→N+1، rollback executable، migration backup، update interruption، signing/notarization/SmartScreen behavior.

#### E-043 — Commercial Beta Loop

شراء/entitlement/offline grace/server outage/expiry/refund، مع مستخدم خارجي يصل إلى successful export بلا تدخل المطور.

#### E-044 — Internal Extension Registry

جميع built-in FX/tools عبر Contribution descriptors نفسها، وAgent/Inspector يولدان discovery منها.

#### E-045 — Native Plugin Isolation Spike

Stable C ABI داخل host منفصل؛ ABI N-1↔N تعمل أو ترفض قبل التشغيل، وfault injection لا تسقط Main App.

#### E-046 — Wasm Hostility and Determinism

Infinite loop/memory bomb/filesystem/network attempts ضمن budgets، ونفس input ينتج command/revision digest متطابقة.

#### E-047 — Plugin Pinning and Supply Chain

Tamper/revocation/rollback/freeze/capability escalation كلها ترفض، وتحديث plugin لا يغير project output بلا migration صريح.

#### E-048 — Mobile Architecture/Policy Canary

فتح semantic project وrender fixture على iOS/Android probe، مع إثبات أن extension manifest تعمل content-only وفق سياسات المتجر.

### 23.28 الحكم المؤقت لهذه الجولة

> يجب أن يبدأ ReFusion كمنتج قابل للتثبيت من `G1`، وأن يبيع في `G6` Creator Loop ضيقة لكنها كاملة، بينما تتطور النواة عبر Layer/Property/Effect/Command contracts موحدة. كل قدرة تعبر UI وAgent وPreview وExport وPersistence وTests وPackaging قبل اعتبارها منجزة. Desktop macOS/Windows هو المسار الحرج، وmobile يبقى portable canary حتى يثبت المنتج. v1 تسجل built-ins عبر Extension Registry لكنها لا تجمد Public C++ ABI؛ SDK المحمولة تأتي في Worker معزول، وC++ لاحقًا فوق Stable C ABI داخل Plugin Host منفصل.

لا تتحول هذه الغربلة إلى Master Plan قبل اعتماد persona وCreator Loop وNot‑V1 ومراجعة الترخيص وإجراء kill-risk spikes المحددة. وعند كتابة الخطة، تنظّم بالـGates والنتائج القابلة للتثبيت والبيع، لا بحسب عدد subsystems.

---

## 24. غربلة Studio Shell والـInspector وتعديل ملفات المشروع خارجيًا

> **حالة هذا القسم: غربلة Draft 0.7 غير معيارية وغير قابلة للاستخدام كمواصفة UI أو Schema تنفيذية.** التصور البصري المصاحب أداة مراجعة فقط؛ ليس Web App، ولا مصدرًا لأبعاد نهائية، ولا جزءًا من المنتج.

### 24.1 التصحيح الأساسي: الأيجنت ليس زرًا داخل التطبيق

المقصود من Agent-native في ReFusion ليس إضافة chat box أو زر «Agent» إلى شريط الأدوات. النموذج المرجح حاليًا:

- ReFusion Studio تطبيق Desktop أصلي، والمرشح الحالي لواجهته Qt Quick/QML.
- الأيجنت عميل خارجي شبيه بـCodex يفتح حزمة المشروع ويقرأ ملفاتها ويعدّلها مباشرة.
- لا يحتاج التطبيق إلى معرفة اسم الأيجنت أو امتلاك واجهته.
- لا يتلقى Canvas أو Inspector أو Timeline أوامر مباشرة من الأيجنت.
- تتغير الواجهة فقط عندما يقبل المحرك لقطة المشروع الجديدة وينشر `Accepted Project Revision`.
- يمكن إضافة تكامل اختياري أو CLI مساعد لاحقًا، لكنه ليس الطريق الوحيد ولا مصدر الحقيقة.

التصور HTML المستخدم في هذه الجولة يحاكي الشكل والتفاعل لتسهيل المراجعة فقط. المسار المستبعد هو جعل HTML/CSS/JavaScript طبقة المنتج أو جعل HTML Canvas محرك العرض.

### 24.2 توزيع Studio Shell المرشح

| المنطقة | الحجم المبدئي للغربلة | المسؤولية | ما لا تملكه |
|---|---:|---|---|
| الشريط العلوي | ارتفاع مدمج | Undo/Redo، project status، export، أوامر التطبيق العامة | لا يضم Agent chat أو حقيقة Layer |
| Insert Rail الأيسر | `56–72` logical px | إنشاء Video/Image/Text/Background/Shape/Audio/SVG | لا ينشئ object محليًا ولا يعدّل Timeline مباشرة |
| مساحة الوسط | مرنة | Canvas toolbar، viewport، guides، zoom، Timeline قابلة لتغيير الارتفاع | لا تملك Project Graph ولا GPU resources |
| Inspector الأيمن | `320–400` px مبدئيًا، قابل للتغيير | تحرير خصائص الـLayer المحددة حسب schema/capabilities | لا يحتفظ بنسخة مستقلة من الخصائص |
| Timeline السفلي | `180–360` px مبدئيًا، قابل للتغيير | tracks، clips، playhead، keyframes، selection | لا تختار frames ولا تفك ترميز الفيديو بنفسها |

هذه الأرقام ليست Design Tokens معتمدة. يجب إثباتها عند window widths وDPI وRTL/LTR مختلفة في E-056.

زر الإدراج في الشريط الأيسر هو **Command Factory** فقط:

```text
Click Insert Text
      │
      ▼
CreateLayer(kind_id="text", defaults, insertion_policy, expected_revision)
      │
      ▼
Accepted Revision + selected_layer_id = new LayerId
```

لا ينشئ الزر QML object يمثل حقيقة المشروع، ولا يضيف صفًا إلى Timeline محليًا ثم يحاول مزامنته لاحقًا.

### 24.3 عقد التعديل المباشر لملفات المشروع

حتى يستطيع Codex التعديل المباشر باحتراف، يجب التفريق بين حقيقتين متطابقتين عند الاستقرار:

1. **Durable Project Files:** التمثيل الدائم القابل للقراءة والعنونة والتعديل على القرص.
2. **Accepted Project Revision:** اللقطة التشغيلية immutable التي يعتمدها المحرك والواجهات والـrender.

أي ملفات جرى تعديلها خارجيًا لكنها لم تمر التحقق تسمى `Disk Candidate` وليست Revision مقبولة بعد.

```text
External Agent
    │
    ├─ writes temporary replacements
    ├─ atomically replaces touched project files
    └─ writes commit marker last
                │
                ▼
        Project File Ingestor
   rescan + parse + migrate + hash
                │
                ▼
       Semantic Diff Candidate
                │
      validate expected/base revision
                │
       ┌────────┴────────┐
       ▼                 ▼
Rejected Candidate   Accepted Revision
diagnostics only          │
                     ┌────┼────┐
                     ▼    ▼    ▼
               Inspector Canvas Timeline
```

الـcommit marker المرشح يحتوي على الأقل:

- `transaction_id` فريد.
- `base_revision_id` الذي قرأه الأيجنت قبل التعديل.
- `schema_version`.
- قائمة الملفات المتأثرة وhash لكل ملف.
- هوية مصدر للتدقيق مثل `external-agent` من دون أن تغيّر الدلالة.
- علامة اكتمال تكتب بعد استبدال كل الملفات المستهدفة.

المراقب filesystem watcher هو **جرس استيقاظ فقط**. لا يجوز اعتبار event واحدة transaction boundary، لأن أحداث الملفات قد تتجمع أو تصل عند rename/remove. بعد الاستيقاظ يعيد File Ingestor فحص الـmanifest/marker والملفات المتأثرة ويعيد ربط المراقبة عند الحاجة.

الحفظ من داخل ReFusion يستخدم المسار العكسي:

```text
UI Command
   │
   ▼
Accepted Revision
   │
   ▼
Persistence Coordinator
temp write + atomic replace + commit marker
   │
   ▼
Durable Project Files
```

هذا لا يجعل القرص بطيئًا داخل كل حركة pointer. أثناء drag توجد `Interaction Transaction` تشغيلية مؤقتة، ثم Revision واحدة وpersist واحدة عند commit؛ Escape يلغيها.

### 24.4 قاعدة التكافؤ بين UI وAgent

الـUI والأيجنت لا يحتاجان الواجهة نفسها، لكن يجب أن يصلا إلى **الدلالة نفسها**:

```text
UI property edit ───────────────┐
                               ├─> Canonical Semantic Change ─> Accepted Revision
Agent project-file edit ────────┘
```

إذا غيّر المستخدم `text.fill.color` إلى الأزرق من Inspector، أو غيّر Codex القيمة نفسها في ملف الـLayer، يجب بعد التطبيع أن تتطابق:

- LayerId وPropertyId.
- القيمة typed والـcolor space.
- keyframe/animation semantics إن وجدت.
- revision semantic digest، باستثناء audit metadata المسموح باختلافها.
- Canvas وTimeline وInspector وExport.

لا يوجد «Agent state» موازٍ، ولا يقوم الأيجنت بتحريك color picker. الـInspector يلاحظ الـAccepted Revision الجديدة ويعرض الأزرق لأنه View مشتقة من المشروع.

### 24.5 Selection Contract

الـSelection حالة Studio Session وليست Project content:

```text
SelectionState = {
  selected_layer_ids,
  primary_layer_id,
  current_revision_id,
  focus_context
}
```

- النقر على Layer في Canvas أو Timeline يحدث SelectionState موحدة.
- الـInspector يشتق محتواه من `primary_layer_id + accepted_revision_id`.
- اختيار Layer لا ينتج Project Revision ولا يكتب ملفات المشروع.
- تغيير الأيجنت لخاصية لا يغيّر اختيار المستخدم.
- إذا حذف الأيجنت الـLayer المحددة، ينتقل الاختيار حتميًا إلى أقرب sibling صالح أو إلى none وفق سياسة تختبر في E-054.
- zoom وpanel widths والأقسام المفتوحة وworkspace layout تفضيلات جلسة/مستخدم، لا حقيقة مشروع.

يمكن الاستفادة من model/selection primitives في Qt، لكن هوية الـSelection عبر Canvas وTimeline وInspector يجب أن تكون عقد ReFusion، لا index خاصًا بـWidget واحدة.

### 24.6 Schema-Driven Inspector

المسار المرجح هو Inspector مولدة من `LayerDescriptor + Capability Registry + Property Schema`، لا `switch(layerType)` ضخم داخل QML.

كل property descriptor يحتاج مبدئيًا:

```text
property_id
value_type
default_value
unit / range / step
group_id / order
editor_hint
animatable
visibility_rule
enablement_rule
capability_requirement
validation / diagnostics
serialization_version
```

QML يعرض `InspectorModel` مشتقًا من Revision immutable عبر models/delegates. محررات متخصصة مثل font picker وgradient editor وBezier mask editor وcurve editor مسموحة، لكنها تصدر canonical commands نفسها ولا تملك property state مستقلة.

ترتيب الأقسام المشترك المقترح:

1. Layer header: الاسم، النوع، enabled، locked، diagnostics.
2. Source/Content: خاص بالنوع.
3. Timing.
4. Layout/Transform.
5. Appearance.
6. Adjustments عند الأنواع الداعمة.
7. Masks/Mattes.
8. Effects stack.
9. Animation/Automation.
10. Advanced/Diagnostics.

كل property قابلة للأنيميشن تعرض authored value عند الزمن الحالي، حالة keyframe، وزر diamond موحدًا. لا تُبنَى منظومة Animation منفصلة للنص وأخرى للفيديو.

### 24.7 قدرات الأنواع المطلوبة في v1

| Layer | Source/Content الخاص | القدرات المشتركة أو المتخصصة في Inspector |
|---|---|---|
| Video | asset، stream، trim، crop، speed | Transform، opacity/blend، adjustments، masks، FX، animation، decode diagnostics |
| Image | asset، crop، fit/fill | Transform، appearance، adjustments، masks، FX، animation |
| Text | content، font identity/fallback، size، tracking، leading، alignment، fill/stroke | Transform، masks، FX، animation، text-on-path لاحقًا إن لم يدخل v1 |
| Background | solid/gradient/media source | fill، transform عند الحاجة، FX، animation |
| Shape | geometry/path، fill، stroke، corner parameters | Transform، masks، FX، animation |
| Audio | asset/stream، trim، channels | gain، pan، routing، restoration/DSP، automation؛ لا visual FX |
| SVG | asset أو editable vector tree حسب النطاق | fill/stroke overrides، Transform، masks، FX، animation مع diagnostics للميزات غير المدعومة |

تصنيف بعض الخصائص مهم كي لا تتكرر دلالتها:

- Rounded corners: coverage/clip primitive أو property appearance مشتركة، لا effect خاص بكل Layer.
- Border/Stroke: appearance processor typed عندما يكون المعنى visual outline.
- Drop Shadow وGlow وBlur: `EffectInstance` مشتركة لأي `VisualSurface` متوافق.
- Color adjustment: effect/processor داخل chain موحدة للفيديو والصورة، مع إعلان color contract.
- Text fill/stroke: خصائص مصدر النص، بينما shadow/glow بعد إنتاج الـvisual surface.

### 24.8 سلوك التحرير داخل Inspector

QML property bindings مناسبة لتحديث العرض عندما يتغير model، لكنها ليست Project Authority. العقد المرشح:

```text
Accepted Revision Snapshot
        │ read-only roles
        ▼
InspectorModel ──> QML Delegate/Editor
                         │ user intent
                         ▼
                 PropertyEdit Command
                         │
                         ▼
                 Command Engine
```

قواعد interaction:

- Text commit يمكن دمجه ضمن transaction واضحة بدل Revision لكل حرف بلا حدود.
- Slider/drag يبدأ preview transaction، يحدث preview override في engine session، ثم ينتج commit واحدة عند release.
- Escape يلغي preview ويرجع إلى Accepted Revision.
- Undo/Redo يعمل على semantic commands، لا على تغييرات Widgets.
- الخاصية المعطلة تعرض سببًا واضحًا مثل unsupported backend أو missing font/plugin، ولا تختفي القدرة بصمت.
- v1 يبدأ single selection؛ multi-selection يحتاج mixed values وbatch command semantics مستقلة ولا يدخل تلقائيًا.

### 24.9 Canvas وقياسات المشروع

الانتقال بين 16:9 و9:16 ليس تغيير CSS ولا تبديل محرك:

- `Composition.width/height/pixel_aspect` خصائص مشروع typed.
- Canvas viewport تقوم fit/zoom/letterbox فقط حول أبعاد الـComposition.
- Portrait وLandscape وSquare تستخدم Project Frame Evaluator نفسه.
- guides وselection handles overlays جلسة لا تدخل frame النهائية.
- Canvas لا تقرأ ملفات المشروع مباشرة؛ تقرأ frame/scene output من accepted revision.

### 24.10 شكل حزمة ملفات مرشح — غير معتمد

الغرض من هذا الشكل إثبات قابلية تعديل Codex، لا اختيار JSON أو أسماء نهائية:

```text
Project.refusion/
├── manifest.*              # schema + accepted/base revision metadata
├── composition.*           # size + timebase + color policy
├── layers/
│   ├── layer_<stable-id>.* # type + source + typed properties
│   └── ...
├── effects/                # effect instances/graphs when split is useful
├── assets.*                # immutable asset references + hashes
├── automation/             # curves/keyframes when split is useful
└── .refusion/
    ├── commits/            # completed external-edit markers
    ├── recovery/           # last-known-good/journal metadata
    └── diagnostics/        # derived; not project truth
```

شروط القابلية للأيجنت:

- UTF-8 وstable IDs وcanonical property names.
- Schema منشورة آليًا ويستطيع الأيجنت الاستعلام عنها.
- ترتيب/formatting ثابت يقلل diffs غير الدلالية.
- binary blobs وcaches خارج الملفات التي يعدلها الأيجنت عادة.
- unknown fields محفوظة أو ترفض وفق version policy واضحة، لا تسقط صامتًا.
- asset originals لا تتغير.

### 24.11 التعارض والحفظ والاسترداد

- كل تعديل خارجي يعلن `base_revision_id`؛ إذا أصبح stale فلا يوجد silent last-writer-wins.
- إن أمكن rebase آمن على PropertyIds مختلفة ينفذ مع سجل واضح، وإلا يرفض المرشح ويُطلب إعادة القراءة.
- partial multi-file state لا يُعرض في Canvas.
- disk-full أو parse failure أو hash mismatch يبقيان Last Known Good Revision فعالة ويظهران diagnostic.
- الكتابة النهائية تستخدم temporary file + atomic replacement حيث تدعم المنصة، مع اختبارات انقطاع الطاقة/crash.
- lock files أدوات تنسيق بين العمليات المتعاونة وليست حماية كافية من أي محرر خارجي؛ `base_revision_id + hashes + commit marker` تبقى ضرورية.
- لا تُراقب آلاف الملفات عشوائيًا؛ يراقب root/manifest/commit area ويعاد المسح الحتمي عند event.

### 24.12 Qt/QML boundary المرجحة

```text
Qt Quick/QML
├── Studio Shell layouts
├── Controls + delegates
├── accessibility/focus/keyboard
└── read-only view models + intent events
           │
           ▼
C++/Rust Bridge
├── InspectorModel / TimelineModel / SelectionModel
├── Command Adapter
├── File Ingestor + Persistence Coordinator
└── native render presentation bridge
           │
           ▼
ReFusion Engine
```

`QAbstractItemModel` أو نماذج مشتقة مرشح مناسب لعرض property groups وtracks في QML. يفصل model/view/delegate البيانات عن العرض، لكن write-through المباشر من delegate إلى model لا يعتمد كممر مشروع؛ delegate ترسل intent إلى Command Adapter.

### 24.13 نطاق v1 المرشح لهذه الواجهة

يدخل v1:

- Insert Rail للأنواع السبعة المذكورة.
- Canvas واحدة تدعم مقاسات Composition المختلفة.
- Timeline وInspector متزامنتان مع single selection.
- Inspector مولدة من schema مع specialized editors محدودة.
- Transform/appearance/mask/FX/animation المشتركة وفق capabilities.
- Agent-readable/editable project package وatomic external-edit ingestion.
- تحديث حي بعد قبول التعديل الخارجي من دون زر Agent داخل التطبيق.
- keyboard focus وshortcuts وRTL/LTR وaccessible names منذ البنية الأولى.

يؤجل مبدئيًا:

- Agent chat مدمج داخل ReFusion.
- multi-selection/mixed values إن هدد Creator Loop الأولى.
- custom arbitrary plugin UI داخل Inspector قبل تثبيت descriptor/delegate contract.
- full workspace customization وundocking غير المحدود.
- vector-node editor كامل لـSVG إن لم يكن مطلوبًا للـCreator Loop.

### 24.14 المسارات المستبعدة مبدئيًا

| المسار | الحالة | سبب الاستبعاد من الدور |
|---|---:|---|
| زر Agent أو chat كمسار الحقيقة | `X` | الأيجنت خارجي ويعدل ملفات المشروع؛ وجود زر يخلط المنتج مع عميل معين |
| Agent يكتب Widgets أو Canvas مباشرة | `X` | يتجاوز validation والـRevision ويكسر Inspector/Timeline parity |
| HTML/Electron كواجهة المنتج بسبب التصور | `X` | التصور أداة مراجعة فقط، والاتجاه الحالي Native Qt/QML |
| Inspector hard-coded لكل Layer في QML | `X` | يكرر property/effect semantics ويعطل التوسع والـplugins |
| property state محفوظة داخل QML controls | `X` | تنشئ حقيقة ثانية لا يراها الأيجنت أو Export |
| filesystem event = commit | `X` | الأحداث قد تتجمع أو تنتج عن replace/rename ولا تثبت اكتمال عدة ملفات |
| قبول ملفات جزئية قبل اكتمال transaction | `X` | يعرض حالة هجينة ويكسر atomic revision |
| Selection مخزنة داخل Project Revision | `X` | تضخم history وتجعل نقر المستخدم تعديلًا للمشروع |
| Revision لكل pixel أثناء drag | `X` | يلوث history ويزيد write/amplification؛ interaction transaction أدق |
| FX مكررة حسب نوع Layer | `X` | يمنع نظام capabilities وtyped ports الموحد |

### 24.15 مصفوفة الترجيح الحالية

| القرار | الحالة | الملاحظة |
|---|---:|---|
| Qt Quick/QML للـStudio Shell | `L + E` | مناسب للـdesktop native والنماذج والـaccessibility؛ يحتاج إثبات render bridge والأداء |
| Schema-driven Inspector | `L + E` | شرط للتوسع وتكافؤ Agent/UI؛ يحتاج prototype بلا switches |
| Agent خارجي يعدل Project Package مباشرة | `P + L + E` | مطلب منتج؛ يحتاج protocol ذري وتجارب تعارض واسترداد |
| Commit marker + base revision + hashes | `L + E` | يمنع partial/stale ingestion؛ الصيغة النهائية لم تحسم |
| Selection كحالة جلسة موحدة | `L + E` | تمنع تلويث المشروع؛ تحتاج سلوك delete/reorder/multi-view |
| Single selection في v1 | `L` | يقلل مخاطرة أول إصدار؛ يعاد فتحه إذا كانت Creator Loop تحتاج multi-select |

### 24.16 تجارب وبوابات الإثبات

#### E-049 — External Agent File Round-Trip

يفتح التطبيق مشروعًا، يعدّل Codex Layer Text في الملفات مباشرة، يقبل File Ingestor التغيير، ويتحدث Inspector وCanvas وTimeline من Revision واحدة من دون زر Agent أو restart.

#### E-050 — Atomic Multi-File Ingestion

تعديل Layer وEffect وAutomation في transaction واحدة مع قتل العملية بعد كل خطوة محتملة؛ لا تظهر حالة جزئية، ويظل Last Known Good صالحًا حتى commit marker مكتملة.

#### E-051 — UI/File Semantic Digest Parity

التعديل نفسه عبر Inspector وعبر ملفات المشروع ينتج canonical semantic digest متطابقة مع السماح باختلاف audit source فقط.

#### E-052 — Generated Inspector Without Layer Switches

إضافة LayerDescriptor وPropertySchema تجريبيين من registry؛ يظهر Inspector ويقبل commands ويحفظ/يعيد الفتح من دون إضافة `if/switch` خاص بالنوع إلى Studio Shell.

#### E-053 — Interaction Transaction and Undo

سحب scale مدة عشر ثوانٍ يعطي preview سلسة، commit دلالية واحدة عند release، cancel صحيح عند Escape، وUndo/Redo مطابقين.

#### E-054 — Selection Under External Mutation

الأيجنت يغيّر/يعيد ترتيب/يحذف Layer محددة بينما المستخدم يعمل؛ لا يوجد dangling selection أو Inspector قديمة، والتعارض يملك نتيجة حتمية.

#### E-055 — Cross-View Accepted-Revision Consistency

بعد 10,000 تعديل UI وخارجي متعاقب ومختلط، تعرض Timeline وInspector وCanvas revision id نفسها ولا توجد property value من Revision أقدم.

#### E-056 — Desktop Layout, RTL and Accessibility Matrix

اختبار macOS وWindows على DPI وأحجام نافذة مختلفة، keyboard-only، screen reader metadata، RTL/LTR، Inspector/Timeline resizing، وعدم قص Insert Rail أو property editors.

### 24.17 الفصل بين Project Authoring وNative C++ Development

عبارة «الأيجنت يكتب Native C++ وتظهر النتيجة لحظيًا» تخفي ثلاث عمليات مختلفة، ولا يجوز إعطاؤها الوعد نفسه:

| المسار | ما يكتبه الأيجنت | زمن الظهور المتوقع | هل يحتاج build؟ |
|---|---|---|---:|
| Project Authoring | Layers وproperties وkeyframes وmasks وFX داخل تمثيل مشروع typed يفهمه المحرك | بعد قبول Revision التالية أثناء تشغيل التطبيق | لا |
| Native Extension Development | C++20 source لإضافة Effect/Tool/Layer producer جديدة عبر SDK | بعد compile + conformance + activation آمنة | نعم |
| Core Engine Development | كود Command/Render/Media/Audio engine نفسه | بعد build واختبارات وإعادة تشغيل النسخة المطورة | نعم، ولا يعد hot-edit للمشروع |

الاختيار المرجح هو **ألا يكون محتوى المشروع العادي C++ خامًا**. تغيير اللون أو إضافة Layer أو keyframe لا يجب أن يستدعي compiler أو يحمل كودًا داخل التطبيق. يكتب الأيجنت representation أصلية للمحرك، typed وversioned وقابلة للتحقق، ثم تمر عبر File Ingestor وCommand Engine كما سبق.

أما إضافة قدرة Native جديدة فتستخدم مسارًا منفصلًا:

```text
Agent writes C++20 extension source
              │
              ▼
Out-of-process Build Orchestrator
compile + link + schema extraction
              │
     ┌────────┴────────┐
     ▼                 ▼
Structured Error   Conformance Tests
file/line/code          │
     │                  ▼
Agent fixes source   Versioned Extension Bundle
                        │
                        ▼
              Extension Supervisor
              side-by-side activation
                        │
                        ▼
              Isolated Plugin Host
                        │
                        ▼
        Capability Registry Revision
              │                 │
              ▼                 ▼
     Inspector descriptors   Engine evaluator
              └────────┬────────┘
                       ▼
             Accepted Project Revision
```

قواعد هذا المسار:

- لا يُحمّل source code أو library جديدة داخل UI Process أو Command Engine أو Render Engine الأساسي.
- البناء يجري خارج العملية مع toolchain/SDK version مثبتين وقابلين لإعادة الإنتاج.
- compiler وlinker وschema وconformance diagnostics تعود بصيغة machine-readable تتضمن file/line/column/code/message/notes.
- عند خطأ syntax أو compile أو ABI أو schema، لا تتغير Capability Registry ولا المشروع الجاري؛ تبقى آخر نسخة سليمة فعالة.
- عند نجاح البناء، تنتج bundle جديدة بهوية وإصدار وhash ولا تستبدل النسخة الفعالة في مكانها.
- Extension Supervisor يحمل النسخة الجديدة side-by-side داخل Plugin Host معزول، يفحص handshake والـABI والـcapabilities ثم يفعّلها عند safe boundary.
- إذا crash الـPlugin Host أو تجاوز budget، يُعزل أو يعاد تشغيله وتصبح العقد المتأثرة `Unresolved/Unavailable` مع diagnostic؛ لا يسقط التطبيق ولا يدّعي نجاحًا.
- بعد نشر Registry Revision جديدة، يعاد توليد Agent catalog وInspectorModel من descriptors نفسها؛ لا يضيف QML panel يدويًا.
- إذا كانت القدرة الجديدة مستخدمة في المشروع، فإن instantiation أو migration الخاصة بها تمر Command Engine وتنتج Accepted Project Revision قبل أن يراها Canvas أو Timeline أو Export.

قناة الأخطاء المقترحة للأيجنت لها واجهتان متكافئتان:

```text
refusion validate/build --diagnostics=json
.refusion/diagnostics/<transaction-or-build-id>.json
```

الأسماء غير معتمدة، لكن العقد المطلوبة هي stable error codes وsource locations وsuggested fixes وcausal chain. يستطيع الأيجنت إعادة القراءة والتعديل والمحاولة باستخدام `base_revision_id` أو `build_id` واضحين.

«لحظي» في Project Authoring يعني propagation bounded بعد atomic save والقبول، لا تحديثًا أثناء كتابة ملف ناقص. أما C++ فليس لحظيًا لكل keystroke؛ زمنه هو compile/test/activate، ويجب وضع budget وقياسه. الوعد المهني هو **لا حالة جزئية، لا سقوط، لا تفعيل عند الخطأ، وظهور موحد فور القبول**، وليس ادعاء zero-time.

#### E-057 — Native Extension Build/Diagnostic Loop

يكتب الأيجنت Effect C++ بها أخطاء syntax وtype/link/schema متعمدة؛ يحصل على diagnostics machine-readable دقيقة، يصلحها، وتنجح bundle من دون تدخل يدوي أو تغيير النسخة العاملة أثناء الفشل.

#### E-058 — Safe Side-by-Side Activation

تحميل N وN+1 داخل Plugin Host مع project مفتوح وتشغيل preview؛ لا mixed ABI أو partial registry، ويتم التحويل عند safe boundary مع rollback إلى N.

#### E-059 — Extension Failure Containment

Crash وhang وmemory/GPU budget violations أثناء build/load/evaluate؛ يبقى Main App وLast Known Good Revision صالحين وتظهر diagnostics قابلة للمعالجة.

#### E-060 — Capability Reflection End-to-End

بعد قبول Extension جديدة، يكتشفها Agent catalog ويولد Inspector خصائصها ويطبقها Command Engine ويعرضها Canvas/Timeline ويصدرها Export من descriptors والـRevision نفسيهما.

### 24.18 الحكم المؤقت لهذه الجولة

> Studio Shell المرجحة هي Qt Quick/QML أصلية: Insert Rail ضيقة يسارًا، Canvas/Timeline في الوسط، وInspector عريضة مولدة من schema يمينًا. لا يوجد Agent button داخل التطبيق. يعمل Codex على ملفات مشروع نصية قابلة للعنونة مباشرة، ويحوّل File Ingestor التغيير الذري المكتمل إلى candidate semantic change؛ وحده ReFusion Command Engine يستطيع إنتاج Accepted Project Revision. عندها فقط تتحدث Timeline وInspector وCanvas وPreview/Export من الحقيقة نفسها.

لا يصبح هذا الحكم قرارًا حتى نجاح E-049 إلى E-060 وحسم صيغة Project Package وcommit marker والتعارض وsingle/multi-selection واتجاه الواجهة، إضافة إلى فصل Project Authoring عن Native Extension/Core Development.

---

## 25. غربلة Live Authoring وAgent Feedback Loop الشبيه بروح Unity

> **حالة هذا القسم: غربلة Draft 0.8 غير معيارية وغير قابلة للاستخدام كمواصفة تنفيذ أو وعد أداء.** الغرض هو تحويل تجربة «أعدّل ملفات المشروع فتتحدث الأداة المفتوحة، والخطأ يظهر ويُصلح» إلى عقود ReFusion واضحة من دون نسخ Unity أو ربط الحقيقة بالـUI.

### 25.1 ما الذي نستعيره من Unity وما الذي لا نستعيره؟

المطلوب هو المبدأ التشغيلي التالي:

```text
Agent edits project sources while Studio is open
              │
              ▼
Detect → Reconcile → Import/Build → Validate
              │
       ┌──────┴──────┐
       ▼             ▼
   Diagnostics   Prepared Generation
                       │
                       ▼
             Atomic Revision Activation
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
       Timeline      Canvas      Inspector
```

Unity توثق دورة refresh تبحث عن الملفات المتغيرة، تجمع ملفات الكود للـcompilation، تعيد تحميل scripting domain عند الحاجة، تستورد بقية الأصول ثم تعمل hot reload. كما تفصل source assets عن artifacts المشتقة وتتبع static/dynamic dependencies. هذه مبادئ نافعة للمقارنة، لكن ReFusion لا يحتاج نسخ C# domain reload أو نموذج GameObject.

المطلوب في ReFusion:

- Source files قابلة للقراءة والتعديل من Codex وأدوات أخرى.
- قاعدة IDs واعتماديات وartifacts مشتقة.
- incremental refresh لا يعيد بناء كل شيء.
- آخر Revision سليمة تستمر أثناء الكتابة والبناء أو عند الخطأ.
- Console بشرية وDiagnostics typed للأيجنت.
- UI وAgent وCLI وMCP تصل إلى الخدمات والعقود نفسها.
- لا تنشيط جزئي ولا مزج بين جيلين داخل frame أو audio block.

غير المطلوب:

- compile C++ لكل تغيير لون أو position أو keyframe.
- تحميل كود غير مكتمل داخل Main App.
- reload كامل للمحرك عند كل save.
- جعل نص Console هو API الأيجنت الوحيدة.

### 25.2 الوعد المهني الصحيح

لا يمكن جعل أي Agent «لا يخطئ مطلقًا». الطريقة الاحترافية هي تقليل التخمين ثم حصر الخطأ بحيث يكون:

- مكتشفًا قبل التفعيل قدر الإمكان.
- محددًا بـstable code ومكان وEntity/Property/Time.
- قابلًا للقراءة آليًا والإصلاح وإعادة المحاولة.
- غير قادر على إفساد Active Revision أو بقية المشروع.
- غير مخفي خلف fallback صامت.

الصياغة المرشحة:

> كل تعديل UI أو Agent يدخل Proposed Generation. لا تصبح النتيجة مرئية أو قابلة للتصدير إلا إذا نجح reconcile/import/build/validation والتحضير، ثم فعّل Revision Authority اللقطة ذريًا. عند الفشل تستمر Last-Known-Good وتصدر Diagnostics تربط الخطأ بالمصدر والـID والزمن والمرحلة.

### 25.3 الطبقات الثلاث التي لا يجوز خلطها

```text
Authoring Sources
  project entities, media refs, extension sources
          │
          ▼
Derived Artifacts
  indices, waveforms, shaders, binaries, render IR, caches
          │
          ▼
Active Project Revision
  immutable semantic state used by evaluation and presentation
```

1. **Authoring Sources:** ما يعدله المستخدم أو Codex ويُحفظ ويُنسخ احتياطيًا.
2. **Derived Artifacts:** قابلة للحذف وإعادة البناء؛ ليست حقيقة المشروع.
3. **Active Revision:** الوحيدة التي تستخدمها Timeline وCanvas وInspector وPreview وExport.

قد تكون الملفات على القرص ناقصة مؤقتًا أثناء save أو تحتوي خطأ. لا يعني ذلك أن Active Revision تصبح ناقصة.

### 25.4 الصورة المعمارية المرجحة

```text
UI Command Adapter ───────────────────────────────────────────────┐
                                                                 │
Codex / IDE / Git / External Tool                                │
              │                                                  │
       File Watch Hints                                          │
              │                                                  │
              ▼                                                  │
     ReFusion Live Authoring Service                             │
     ├── Change Reconciler                                       │
     ├── Source/Asset Database                                   │
     ├── Dependency Graph                                        │
     ├── Import/Build Workers                                    │
     ├── Validators + Graph Compiler                             │
     ├── Artifact Store                                          │
     └── Diagnostic/Run Store                                    │
              │                                                  │
              ▼                                                  ▼
        Proposed Generation ───────────────────────> Revision Authority
                                                         │ compare-and-swap
                                                         ▼
                                                Prepared Project Revision
                                                         │
                                                         ▼
                                             Revision Presentation Gate
                                                ┌────────┼────────┐
                                                ▼        ▼        ▼
                                            Timeline   Canvas   Inspector
                                                └────────┼────────┘
                                                         ▼
                                              Preview / Audio / Export

Developer Control Plane
  ReFusion CLI ─┐
                ├──> الخدمات نفسها، لا implementations مكررة
  ReFusion MCP ─┘
```

الـStudio UI لا تستورد ولا تبني ولا تفعّل. تعرض progress/diagnostics/snapshots وترسل intent فقط.

### 25.5 الهويات التشغيلية الأربع

لمنع الخلط بين «الملف تغير» و«المشروع تغير»:

```text
SourceGenerationId   # لقطة مستقرة من ملفات authoring؛ قد تفشل
ArtifactGenerationId # نتائج import/build المشتقة
ProjectRevisionId    # semantic revision مقبولة
EvaluationStamp      # ما يعرض الآن عند زمن محدد
```

```text
EvaluationStamp {
  project_id
  revision_id
  composition_id
  project_time
  transport_sequence
}
```

كل Timeline snapshot وInspector model وCanvas frame تحمل stamp متطابقة. النتيجة المتأخرة من Revision أو زمن قديمين تُرمى ولا تُعرض.

### 25.6 Project Workspace مرشحة — الأسماء غير معتمدة

```text
MyProject.refusion/
├── AGENTS.md
├── CODEX.md
├── CLOUD.md
├── .agents/
│   └── skills/
│       ├── refusion-authoring/
│       └── refusion-extension-dev/
├── .codex/
│   └── config.toml              # project-scoped local MCP configuration
├── Project/
│   ├── project.*
│   ├── compositions/
│   ├── tracks/
│   ├── clips/
│   ├── layers/
│   ├── effects/
│   └── automation/
├── Assets/
├── Extensions/
│   ├── cpp/
│   ├── shaders/
│   └── manifests/
└── .refusion/
    ├── SourceDB.*               # derived local database
    ├── ArtifactDB.*             # derived local database
    ├── Artifacts/<content-hash>/
    ├── Staging/<change-set-id>/
    ├── Runs/<run-id>/
    ├── Diagnostics/
    ├── Recovery/
    └── status/active.*
```

قواعد أولية:

- `Project/` و`Assets/` و`Extensions/` authoring sources.
- `.refusion/Artifacts` وdatabases وruns مشتقة ومحظورة على الأيجنت كهدف تعديل.
- importers لا تولد output داخل source tree كي لا تصنع refresh loops.
- cache key لا يعتمد على path أو timestamp وحدهما.
- لا ينسخ المستخدم أو الأيجنت raw GPU/Skia/C++ runtime objects إلى المشروع.

### 25.7 File Watcher هو Hint فقط

`QFileSystemWatcher` يمكنه الإبلاغ عن modification/rename/remove، وقد تتجمع الأحداث، وتتوقف مراقبة ملف بعد rename/remove، وتوجد حدود موارد حسب النظام. لذلك:

- watcher يقول «أعد الفحص»، ولا يقول «هذه transaction مكتملة».
- Reconciler يقرأ manifest/commit marker ويعيد scan للمجلدات المطلوبة.
- content hash هو دليل التغيير؛ mtime/size optimization فقط.
- atomic save عبر temp+rename يعيد تسجيل المراقبة.
- startup، app focus، manual refresh وwatcher overflow تفعّل reconciliation صريحة.
- الملف الذي يتغير أثناء قراءته لا يدخل build؛ يعاد التقاط snapshot ثابتة.
- `debounce` وحدها ليست ضمانًا لاكتمال عدة ملفات.
- تغييرات Git كبيرة تجمع batch واحدة bounded بدل event storm غير محدود.

### 25.8 المساران المقبولان لكتابة الأيجنت

#### المسار المرجح: Explicit ChangeSet

```text
BeginChangeSet(base_revision_id)
  → allocate/edit stable source files in staging
  → declare touched/deleted files and expected hashes
  → validate
  → optional render probe
  → CommitReady marker
  → activate or return diagnostics
```

```text
ChangeSet {
  change_set_id
  project_id
  base_revision_id
  intent
  touched_files[] { uri, expected_old_hash, proposed_hash }
  deleted_files[]
  requested_policy
}
```

هذا ما ينبغي أن تستخدمه Skill/MCP/CLI لأن transaction boundary صريحة، ولا يمنع أن الأيجنت هو من يكتب الملفات نفسها.

#### مسار التوافق: Direct File Save

إذا كتب Codex أو IDE مباشرة داخل ملفات canonical:

- watcher يجمع burst قصيرة.
- Reconciler يلتقط stable source snapshot.
- dependency-connected files تصبح generation واحدة.
- base revision تستنتج من manifest/commit metadata إن وجدت.
- إن كان الاكتمال غامضًا يبقى الجيل `Collecting` أو يرفض بـdiagnostic؛ لا يفعّل تخمينًا.

المساران يمران import/validation/Revision Authority نفسها. MCP أسرع وأوضح، لكنه ليس حقيقة مشروع بديلة.

### 25.9 حالات Live Authoring Generation

```text
Idle
 → Notified
 → Reconciling
 → SnapshotCaptured
 → ResolvingDependencies
 → Importing / Building
 → Validating
 → Prepared
 → AwaitingSafePoint
 → Activating
 → Active
```

المخارج غير الناجحة:

```text
Superseded           # تغير source أثناء العمل؛ لا يُفعّل الناتج القديم
RejectedUsingLKG     # parse/build/validation فشل؛ LKG تستمر
Conflict             # base revision قديمة ومتعارضة
Quarantined          # importer/plugin انهار أو خالف policy
RequiresRestart      # core ABI/renderer change
Canceled
RecoveryPending
```

يجب وضع حد لعدد refresh restarts. عدم convergence ينتج `REFRESH_DID_NOT_CONVERGE` بدل loop بلا نهاية.

### 25.10 Source/Asset Database وArtifact Database

```text
AssetRecord {
  asset_id
  canonical_uri
  asset_kind
  source_hash
  importer_id
  importer_version
  importer_settings_hash
  dependency_set_hash
  source_state
  active_artifact_id
  last_good_artifact_id
  diagnostic_set_id
}
```

```text
ArtifactRecord {
  artifact_id
  artifact_key
  source_generation_id
  platform_target
  toolchain_fingerprint
  schema_or_abi_version
  dependency_ids[]
  output_hashes[]
  validation_state
}
```

```text
ArtifactKey = hash(
  source bytes
  + importer identity/version
  + import settings
  + static dependencies
  + discovered dynamic dependencies
  + platform/backend/color policy
  + toolchain/contract digest
)
```

- rename لا يعيد هوية Asset.
- حذف cache كاملة يعيد artifacts نفسها دلاليًا.
- dependency change يبطل المعتمدين فقط.
- importer أو toolchain version جزء من المفتاح.
- source وartifact databases محلية مشتقة وليستا ملفات تعاون canonical.

### 25.11 تصنيف التغيير ومسار التفعيل

| نوع التغيير | العمل المطلوب | نقطة التفعيل |
|---|---|---|
| Property/Layer/Keyframe typed | parse + semantic validation + graph invalidation | Revision Presentation Gate |
| صورة/صوت/فيديو | probe/import/index/artifacts | frame/audio safe boundary |
| Timeline topology أو duration | graph rebuild جزئي | project safe point |
| Shader أو FX graph | compile + GPU resource prepare | frame boundary |
| Audio graph | DSP graph prepare | audio block boundary مع crossfade عند الحاجة |
| C++ extension | out-of-process build + isolated shadow host | host routing safe point |
| Importer code | build importer ثم reimport للمتأثرين | generation جديدة |
| Core engine/ABI/backend | developer rebuild | Restart Required |

التعديل الدلالي العادي لا يستدعي C++ compiler. C++ يظهر فقط عندما يضيف الأيجنت Capability Native جديدة أو يطور المحرك.

### 25.12 Revision Authority وPresentation Gate

بعد نجاح generation:

1. يُبنى Project snapshot وrender/audio execution plans خارج المسار النشط.
2. تنجح schema/reference/time/color/capability validation.
3. تُحضّر الموارد اللازمة لأول frame/audio block.
4. تقارن Revision Authority `base_revision_id` بطريقة compare-and-swap.
5. تحضر Timeline وInspector وCanvas `Ready(EvaluationStamp)`.
6. يبدّل Presentation Gate الـvisible stamp دفعة واحدة.
7. تبقى Revision السابقة ومواردها حية حتى انتهاء القراء وGPU fences.

محظور:

- Inspector جديدة مع Canvas قديمة بلا status صريح.
- frame ناتجة من Revision قديمة بعد تفعيل الجديدة.
- audio block يجمع processors من جيلين.
- Timeline تحاول «إخبار» Canvas بما تغير؛ كلاهما يقرأ snapshot نفسها.

يجوز عرض preview منخفضة الجودة أولًا إذا حملت stamp الجديدة نفسها. تحسينها لاحقًا لا يغير دلالة المشروع.

### 25.13 عقد الهوية الدقيقة

```text
ProjectId
CompositionId
TrackId
ClipId
LayerId
AssetId
EffectId
MaskId
PropertyId
RevisionId
ChangeSetId
RunId
```

القواعد:

- IDs typed وopaque بطول ثابت مناسب؛ خوارزمية التوليد النهائية تحتاج قرارًا.
- لا تُشتق من display name أو path أو ترتيب أو index.
- rename/reorder/save/reopen لا تغير الهوية.
- ID المحذوف لا يعاد استخدامه.
- duplicate ينشئ IDs جديدة للكائنات المملوكة.
- المراجع تحمل `project_id + typed_id`، ولا تعبر المشاريع بلا أمر import/link صريح.
- الأيجنت لا يخترع IDs؛ يستخدم ID allocator من CLI/MCP أو IDs ولّدها المحرك داخل transaction.
- `EffectId` هو instance، أما النوع فهو `EffectDescriptorId@Version`.

فصل الكيانات:

```text
Track = حاوية مرتبة في domain مرئي أو صوتي
Clip  = موضع ومدى زمني داخل Track
Layer = المحتوى والخصائص والمؤثرات التي يقيّمها المحرك
```

Split لا يشارك Layer state عرضيًا. يمكن للجزأين الاستمرار في الإشارة إلى `SourceAssetId` نفسها، لكن Clip/Layer identities الجديدة صريحة.

### 25.14 Pixel-True Canvas

```text
CanvasSpec {
  width_px
  height_px
  pixel_aspect_ratio
  working_color_space
  alpha_mode
  render_precision
  origin: TopLeft
  x_direction: Right
  y_direction: Down
}
```

الحكم المرشح:

- `1 Composition Unit = 1 Composition Pixel`.
- حدود الـCanvas المستمرة: `[0,width) × [0,height)`.
- pixel cell `(i,j)` هي `[i,i+1) × [j,j+1)` ومركزها `(i+0.5,j+0.5)`.
- الخصائص قد تكون subpixel؛ التقريب يحدث عند rasterization بسياسة معلنة ولا يعاد حفظ الناتج المقرب.
- resolution لا تُستنتج من window size أو QML logical pixels.
- zoom/pan/fit/DPI ليست Project State.
- `100% Pixel Preview` فقط يعني composition pixel واحدة تقابل device pixel في drawable الفعلية؛ على HiDPI لا تكفي UI logical point.

فضاءات الإحداثيات:

```text
SourcePixelSpace
 → SourceDisplaySpace
 → LayerLocalSpace
 → ParentLayerSpace
 → CompositionSpace
 → ViewportSpace
 → DevicePixelSpace
```

- Source metadata مثل orientation/clean aperture/pixel aspect تطبق بين SourcePixel وSourceDisplay.
- Property/Mask/Crop schema تعلن space وunit؛ لا توجد `position` غامضة.
- Viewport/Device spaces للعرض فقط ولا تحفظ.
- hit testing يستخدم inverse لسلسلة التحويل نفسها.
- transform غير قابلة للعكس تنتج diagnostic ولا تخمّن hit-test بديلة.
- الأيجنت يستدعي `measure_layer(layer_id, time, space)` للحصول على bounds بعد font/layout/masks/FX بدل تقديرها من font size.

### 25.15 Timeline-Time Truth

```text
ExactTime {
  numerator
  denominator
}

TimeRange = [start, end_exclusive)

FrameRate {
  numerator
  denominator
}

AudioSampleRange {
  start_sample
  end_sample_exclusive
  sample_rate
}
```

القواعد:

- لا binary floating seconds كمصدر حقيقة.
- كل العمليات rational/integer checked ومطبعة، ولا denominator صفر.
- composition frame `n` يغطي `[n/fps,(n+1)/fps)`.
- audio sample `n` يغطي `[n/sampleRate,(n+1)/sampleRate)`.
- seconds/timecode واجهة عرض، وليست عنوانًا canonical إذا كان domain غامضًا.
- أمر `frame 100` يحدد Composition/Source domain؛ الغموض يرفض بـ`TIME_DOMAIN_AMBIGUOUS`.
- Clip تحفظ `project_range + source_time_map`.
- VFR تستخدم Presentation Index وPTS/duration، لا `seconds × averageFPS`.
- B-frames ترتب للعرض حسب PTS، لا decode callback order.
- missing/contradictory timestamps تبقى Unknown/Derived/Offline وفق policy معلنة، لا تُختلق.

مثال Agent دقيق:

```text
track_id: trk_...
clip_id: clp_...
project_range:
  start: 15015/30000 seconds
  duration: 90090/30000 seconds
display:
  start_frame: 15
  end_frame_exclusive: 105
  frame_rate: 30000/1001
```

المثال توضيحي فقط؛ لا تُشتق rational الأصلية من النص المعروض بعد الحفظ.

### 25.16 Console وDiagnosticEnvelope

الـConsole المقترحة Drawer/Panel بشرية قابلة للفتح داخل Studio، وليست Agent chat ولا Project Authority. نفس الأحداث تحفظ كبيانات منظمة:

```text
DiagnosticEnvelope v1 {
  schema_version
  diagnostic_id
  fingerprint
  run_id
  change_set_id?
  source_generation_id?
  base_revision_id?
  active_revision_id
  phase
  severity
  blocking
  code
  tool
  tool_code?
  message
  project_id
  entity_refs[]
  property_ref?
  time_range?
  coordinate_space?
  primary_location { uri, line, column, range }
  related_locations[]
  fixits[]
  recovery_options[]
  retryable
  suggested_action
  help_uri?
  raw_log_ref?
}
```

مراحل التشخيص:

```text
reconcile, parse, import, validate, configure, compile, link,
package, ABI, launch, handshake, capability, runtime, render,
audio, performance, activation, recovery
```

عائلات رموز مبدئية:

```text
RFX-ID-*       RFX-REF-*      RFX-REV-*
RFX-TIME-*     RFX-CANVAS-*   RFX-ASSET-*
RFX-SCHEMA-*   RFX-CXX-*      RFX-LINK-*
RFX-ABI-*      RFX-HOST-*     RFX-GPU-*
RFX-RENDER-*   RFX-AUDIO-*    RFX-RECOVERY-*
```

كل Run تنتج bundle مشتقة غير قابلة للتعديل:

```text
run.json
events.jsonl
diagnostics.json
artifacts.json
stdout.log
stderr.log
render-report.json
crash/
```

الأيجنت يقرأ diagnostics أولًا، ثم raw logs بالـcorrelation ID عند الحاجة. `fingerprint` يمنع loop إصلاح يعيد الخطأ نفسه بلا تقدم. الأسرار والـtokens ومحتوى media والمسارات الشخصية تُحجب في export الافتراضي.

### 25.17 سلوك الفشل وعدم تأثيره في بقية المشروع

السياسة المرشحة:

| نوع الخطأ | النتيجة |
|---|---|
| parse/schema/reference/time invariant | رفض ChangeSet كاملة؛ Active Revision السابقة تستمر |
| compiler/link/ABI لExtension جديدة | candidate مرفوضة؛ plugin السابقة تستمر |
| importer/asset failure جديد | رفض افتراضي، أو `UnresolvedNode` فقط إذا طلبت policy صريحة وتسمح schema |
| runtime plugin crash/hang | quarantine للـhost والعودة إلى Last-Known-Good أو تعطيل instance بتشخيص |
| GPU preparation failure | لا swap؛ الموارد القديمة تستمر |
| stale UI/Agent edit | `REVISION_CONFLICT`؛ لا last-writer-wins |
| core ABI change | `RequiresRestart`؛ لا hot activation زائفة |

لا يعيد النظام source الخاطئة تلقائيًا؛ تظل على القرص ليراها الأيجنت ويصلحها. الذي لا يتغير هو Active Revision.

في UI يظهر بوضوح:

```text
Source generation G42 rejected
Preview is using Last-Known-Good Revision R18
3 blocking diagnostics
```

لا يجوز أن يبدو التطبيق كأنه طبق التعديل بينما يعرض silently نسخة قديمة.

### 25.18 Developer Control Plane: CLI وMCP

الـCLI وMCP واجهتان فوق الخدمات نفسها:

```text
Canonical Contracts + Engine Services
                │
       ┌────────┴────────┐
       ▼                 ▼
 ReFusion CLI       ReFusion MCP
 local / CI         live app/runtime
       └────────┬────────┘
                ▼
 Run IDs + Diagnostics + Artifacts + Revision results
```

CLI مرشحة:

```text
refusion doctor
refusion project describe --json
refusion capabilities export --json
refusion edit begin/validate/commit/abort
refusion diagnostics show --json
refusion logs query --jsonl
refusion render probe
refusion extension validate/configure/build/test/package
refusion extension install-candidate/activate/rollback/status
refusion run status/cancel
```

كل command طويلة تعيد `run_id`، وتدعم human output و`--json`/`--jsonl` وstable exit category وcancellation. exit code وحده لا يكفي لإعلان نجاح؛ تقرأ gate results وblocking diagnostics.

### 25.19 MCP كربط وثيق اختياري ومرجح

MCP لا يحل محل الملفات أو Command Engine؛ يمنح الأيجنت discovery وtransactions وsubscriptions منظمة. Resources بيانات سياقية، وTools أفعال typed ذات JSON Schemas.

Resources مرشحة:

```text
refusion://project/{project_id}/status
refusion://project/{project_id}/revision/{revision_id}/snapshot
refusion://project/{project_id}/timeline
refusion://project/{project_id}/layer/{layer_id}
refusion://contracts/catalog/{engine_build_id}
refusion://runtime/capabilities
refusion://runs/{run_id}/diagnostics
refusion://runs/{run_id}/logs
refusion://runs/{run_id}/artifacts
```

Tools مرشحة:

```text
describe_project
resolve_capability
allocate_ids
measure_layer
resolve_time
begin_edit
validate_edit
commit_edit
abort_edit
get_generation_status
query_diagnostics
render_probe
start_extension_build
run_conformance
activate_candidate
rollback_candidate
cancel_run
```

قواعد MCP:

- transport محلي STDIO أو local socket أولًا، لا public HTTP في v1.
- tool لا يقبل raw shell أو path اعتباطية خارج workspace.
- authorization/validation داخل server؛ annotations ليست حماية.
- read resources منفصلة عن write tools.
- resources المهمة تدعم subscription/update notifications إن دعم العميل.
- CLI/MCP تعيدان IDs وdiagnostics دلالية متطابقة.
- MCP adapter لا يستطيع إنتاج Revision من دون Revision Authority.

### 25.20 AGENTS.md وCODEX.md وCLOUD.md والـSkills

وفق سلوك Codex الموثق، `AGENTS.md` تُقرأ تلقائيًا بهرم root→current directory، بينما الأسماء الأخرى لا تُقرأ كتعليمات تلقائية إلا إذا أضيفت fallback أو طلب `AGENTS.md` قراءتها. لذلك:

| السطح | دوره المرشح |
|---|---|
| root/nested `AGENTS.md` | invariants قصيرة، أوامر validation الرسمية، الملفات المحظورة، Definition of Done |
| `CODEX.md` | دليل مفصل لخريطة المشروع، error codes، أمثلة الإصلاح، evidence workflow؛ يربطه AGENTS صراحة |
| `CLOUD.md` | حدود cloud/native runners وGPU/signing/artifact handoff؛ ليس إعداد بيئة تنفيذيًا بحد ذاته |
| `SKILL.md` | workflow متكرر موجّه: inspect→edit→validate→commit→render→fix |
| generated catalog | قدرات وعقود مشتقة من Registry؛ read-only ولا تُنسخ يدويًا داخل التعليمات |
| MCP resources | runtime overlay حي للأجهزة والـplugins والـrevision الحالية |

إذا كان المقصود `CLAUDE.md` لأداة أخرى، يبقى thin adapter يشير إلى العقود canonical بدل نسخ قائمة القدرات. لا تُحافظ فرق متعددة على تعليمات متضاربة يدويًا.

Skill القوية ليست ملفًا هائلًا يحمل كل engine schema. المسار الأصح progressive disclosure:

```text
.agents/skills/refusion-authoring/
├── SKILL.md                    # router/workflow مختصر وصارم
├── references/
│   ├── project-model.md
│   ├── coordinates.md
│   ├── time.md
│   ├── diagnostics.md
│   ├── examples.md
│   └── failure-policies.md
└── scripts/
    └── collect-run-evidence

.agents/skills/refusion-extension-dev/
├── SKILL.md
├── references/
│   ├── cpp-sdk.md
│   ├── lifecycle.md
│   └── conformance.md
└── scripts/
    └── collect-extension-evidence
```

وتلزم الـSkill الأيجنت بـ:

1. قراءة project status وactive revision.
2. اكتشاف schema/capabilities من Registry/MCP/CLI؛ لا تخمين IDs أو Property names.
3. فتح ChangeSet على `base_revision_id`.
4. تعديل authoring sources فقط.
5. validate/dry-run.
6. قراءة blocking diagnostics وإصلاحها.
7. render probe عند زمن/Composition محددين.
8. commit ثم انتظار Active stamp.
9. التحقق من Timeline/Canvas/Inspector revision IDs.
10. عدم إعلان النجاح قبل اكتمال gates.

### 25.21 Generated Contracts وC++ SDK

المصدر المرشح:

```text
Engine Capability Registry + contracts/
├── commands
├── layer/effect/property schemas
├── diagnostics
├── extension manifest
├── MCP schemas
└── ABI/WIT contracts
```

مولّد واحد ينتج:

- C/C++ bindings.
- Agent catalog.
- MCP tool/resource schemas.
- Inspector descriptors.
- SDK documentation.
- compatibility digest.

يفصل:

1. **Contract Catalog:** ثابت لكل engine/SDK build.
2. **Runtime Capability Overlay:** الجهاز والـGPU/backend والـplugins المثبتة والتراخيص؛ يُقرأ live.

الأيجنت ممنوع من تعديل generated outputs لإخفاء drift. CI تعيد التوليد وتقارن digest.

### 25.22 C++ Extension Build وHot Activation

عندما يكتب Codex C++ لإضافة Effect/Tool/Layer producer:

```text
C++ Source
 → Build Worker خارج التطبيق
 → configure/compile/link
 → manifest/schema/ABI validation
 → conformance tests
 → immutable artifact digest
 → Shadow Plugin Host
 → handshake + state migration sandbox
 → deterministic render probe
 → safe routing swap
 → drain old host
```

- build scripts غير موثوقة ولا تعمل داخل ReFusion process.
- build يستخدم preset/toolchain/contract digests مثبتة، و`compile_commands.json` متاح للأدوات.
- لا يُستبدل DLL/dylib محمل في مكانه، ولا `dlclose/FreeLibrary` loop داخل Main App.
- كل candidate تحمل plugin/version/artifact/ABI/target/toolchain/contract IDs.
- failure أثناء probation يعيد Last-Known-Good ويضع candidate في quarantine.
- Core Engine source change تتطلب Developer Build/Restart ولا تتنكر كـProject hot reload.

### 25.23 دورة الأيجنت الكاملة

```text
Load AGENTS + matching Skill
        │
        ▼
Describe Project R18 + Capability Catalog
        │
        ▼
Begin ChangeSet(base=R18)
        │
        ▼
Write files / structured edits
        │
        ▼
Validate + Build needed artifacts
   ┌────┴───────────────┐
   ▼                    ▼
Blocking diagnostics   Prepared G42
   │                    │
Read code/location     Render probe
Fix and retry          │
   └──────────────┐     ▼
                  └─ Commit
                       │
                       ▼
             Wait Active Revision R19
                       │
                       ▼
 Verify stamp across Timeline/Canvas/Inspector
```

إذا تكرر diagnostic fingerprint نفسها مرات متتالية بلا evidence جديد، تتوقف الـSkill وتبلغ blocker بدل حلقة تعديل عمياء.

### 25.24 مثال تطبيقي دقيق

طلب المستخدم: «أضف Text من 2s إلى 6s عند x=960 وy=540 بلون أزرق».

الأيجنت لا يخمّن index أو float time:

1. يقرأ `project_id`, `composition_id`, `active_revision_id`, `CanvasSpec`, `FrameRate`.
2. يطلب `allocate_ids(kind=layer,clip)`.
3. يحدد `TrackId` متوافقة أو ينشئها بأمر typed.
4. يكتب Layer/Clip باستخدام `TimeRange [2/1,6/1)` و`CompositionSpace px` وPropertyIds من catalog.
5. يعمل validate؛ المحرك يفحص font، color type، IDs، الزمن والـbounds.
6. يعمل render probe عند `4/1` ويحصل على frame تحمل `(R-candidate, CompositionId, 4/1)`.
7. يلتزم؛ عند activation يعرض Inspector اللون/الموضع، Timeline المدى، Canvas النتيجة من Revision نفسها.

إذا كان الخط غير موجود:

```text
RFX-FONT-0004
entity: lyr_...
property: text.font_family
source: Project/layers/lyr_....*
message: Requested font identity is unavailable on target
recovery: choose fallback from list_compatible_fonts
blocking: true
```

يقرأ الأيجنت التشخيص، يستعلم عن fonts المتوافقة، يعدل الملف ويعيد validate. R18 تبقى فعالة طوال الوقت.

### 25.25 حدود «اللحظية»

الـlatency ليست واحدة:

| التغيير | فئة الاستجابة المرشحة | الملاحظة |
|---|---|---|
| UI property drag | interactive preview budget | ephemeral revision موحدة ثم commit |
| Project file صغيرة | sub-second target `E` | بعد stable snapshot وvalidation |
| media asset | حسب probe/index/import | تستمر LKG أثناء العمل |
| shader/FX graph | compile-bound | لا تفعيل قبل GPU prepare |
| incremental C++ extension | build-bound | ثوانٍ محتملة، لا وعد frame-time |
| core engine | restart-bound | خارج live project authoring |

الأرقام النهائية توضع بعد benchmarks على أجهزة مرجعية. «فوري» يعني عدم الحاجة إلى restart في Project Authoring، لا zero milliseconds ولا العرض أثناء كتابة ملف ناقص.

### 25.26 المسارات المستبعدة

| المسار | الحالة | السبب |
|---|---:|---|
| file watcher event يفعّل المشروع مباشرة | `X` | لا يثبت اكتمال write أو transaction |
| C++ لكل property/layer edit | `X` | latency وأمان وتعقيد لا داعي لها |
| path/name/index كهوية | `X` | rename/reorder يكسر المراجع ويجبر الأيجنت على التخمين |
| `float seconds` وaverage FPS كحقيقة | `X` | drift وخطأ VFR/frame boundaries |
| Canvas position بوحدة preview/QML | `X` | zoom/DPI/window تغير الدلالة |
| Console text وحده كـAgent API | `X` | غير typed وهش ولا يضمن locations أو codes |
| وضع كل schemas داخل AGENTS/SKILL | `X` | تتقادم وتستهلك السياق وتختلف عن Registry |
| افتراض CODEX.md/CLOUD.md auto-loaded | `X` | غير صحيح دون fallback/تعليمة صريحة |
| CLI وMCP بمنطقين مستقلين | `X` | يخلق نتيجتين وتشخيصين مختلفين |
| raw shell عام عبر MCP | `X` | صلاحيات غير محدودة وغير typed |
| partial activation لـChangeSet | `X` | ينتج مشروعًا هجينًا يصعب فهمه واسترجاعه |
| hot-unload C++ داخل Main App | `X` | state/threads/callbacks قد تبقى مرتبطة بالكود القديم |
| last-writer-wins بين UI وAgent | `X` | فقد صامت للتعديلات |
| إخفاء LKG بلا badge | `X` | يوهم المستخدم بأن المصدر الجديد يعمل |
| Agent fix loop بلا fingerprint/حد | `X` | دوران وتعديلات عشوائية |

### 25.27 تجارب وبوابات الإثبات

#### E-061 — External Edit to Active Revision

Codex يعدّل Project files أثناء فتح Studio؛ يلتقط النظام generation مستقرة، يقبلها، وتتحول Timeline/Canvas/Inspector إلى EvaluationStamp واحدة من دون restart.

#### E-062 — Watcher Loss and Atomic-Save Reconciliation

إسقاط events، temp+rename، 1,000 saves سريعة وGit checkout كبير؛ reconciliation بالhash تكتشف كل فرق ولا تبني ملفًا نصف مكتوب.

#### E-063 — Dependency and Artifact Correctness

تغيير source/importer/settings/static/dynamic dependency يبطل artifacts المتأثرة فقط؛ حذف cache يعيد semantic result نفسها.

#### E-064 — Revision Presentation Gate

تأخير Canvas وTimeline وInspector عشوائيًا خلال 10,000 activation؛ لا يظهر mixed revision/time stamp ولا frame متأخرة.

#### E-065 — Stable Identity Under Project Operations

Rename/reorder/save/reopen/Git move لا تغير IDs؛ split/duplicate تولد IDs صحيحة؛ dangling/cross-project refs ترفض.

#### E-066 — Pixel-True Coordinate Matrix

Checkerboard أحادي pixel وnested transforms/masks/hit-tests على macOS/Windows وDPI/Retina متعددة؛ 100% preview يطابق physical drawable pixels.

#### E-067 — Exact Timeline/Frame/Sample Semantics

`24000/1001`, `30000/1001`, VFR/B-frames/missing PTS و44.1/48/96kHz؛ لا drift أو duplication/gap غير معلن، ولا average-FPS selection.

#### E-068 — Structured Agent Repair Loop

زرع أخطاء IDs/schema/font/time/C++/ABI/render؛ كل خطأ ينتج code/location/entity/fingerprint صحيحًا، ويصل Codex إلى fix أو blocker بلا تعديل generated/cache files.

#### E-069 — Concurrent UI/Agent Editing

UI وAgent من base revision نفسها؛ التغييرات المنفصلة تعاد validation بعد semantic rebase، والمتعارضة ترفض بلا overwrite.

#### E-070 — Last-Known-Good and Crash Recovery

حقن crash/power loss بعد كل مرحلة من snapshot إلى activation؛ بعد restart توجد Revision القديمة أو الجديدة كاملة، وsource الخاطئة تبقى قابلة للإصلاح.

#### E-071 — Native C++ Build and Shadow Activation

compile/link/ABI/runtime failures وألف swap/rollback؛ لا Main App crash ولا mixed binary، والـPlugin السابقة تستمر حتى نجاح candidate.

#### E-072 — AGENTS/Skill/CLI/MCP Contract Parity

إثبات instruction discovery، generated catalog digest، CLI/MCP semantic parity، resource subscriptions، redaction، وأن الأيجنت لا يخمّن ID/property/time/coordinate capability.

### 25.28 الحكم المؤقت لهذه الجولة

> الآلية المرجحة هي `ReFusion Live Authoring Service` تلتقط Authoring Sources وتبني Source/Dependency/Artifact generations خارج الحالة الحية، ثم تسمح لـRevision Authority وحدها بقبول Project Revision وتسمح لـPresentation Gate بتبديل Timeline وCanvas وInspector على EvaluationStamp واحدة. Codex يعمل إما عبر ملفات ChangeSet ذرية أو direct-save reconciliation، ويستخدم CLI/MCP وSkill لاكتشاف العقود وقراءة Diagnostics typed وإصلاحها. Project Authoring لا يحتاج C++ compiler؛ C++ Native Extensions تُبنى خارج التطبيق وتُفعّل داخل Shadow Plugin Host معزول، بينما Core changes تحتاج restart.

لا تصبح هذه الآلية قرارًا أو وعد «100%» قبل نجاح E-061 إلى E-072 وتحديد file formats وID allocator وcoordinate/time policies وlatency budgets وfailure severity وMCP v1 scope. النجاح القابل للضمان هو consistency بعد القبول وcontainment عند الفشل، لا انعدام الأخطاء أو zero-time.

---

## 26. مراجع البحث الأولية

هذه الروابط أدلة بحث أولية وليست مواصفات ReFusion:

- Skia documentation: https://docs.skia.org/docs/
- Skia image filters: https://api.skia.org/classSkImageFilters.html
- SkSL runtime effects: https://docs.skia.org/docs/user/sksl/
- wgpu backends: https://docs.rs/wgpu/latest/wgpu/enum.Backend.html
- Godot RenderingServer: https://docs.godotengine.org/en/latest/classes/class_renderingserver.html
- Qt QRhi: https://doc.qt.io/qt-6/qrhi.html
- Qt QAbstractItemModel: https://doc.qt.io/qt-6/qabstractitemmodel.html
- Qt Quick models, views and delegates: https://doc.qt.io/qt-6/qtquick-modelviewsdata-modelview.html
- Qt QML property binding: https://doc.qt.io/qt-6/qtqml-syntax-propertybinding.html
- Qt QItemSelectionModel: https://doc.qt.io/qt-6/qitemselectionmodel.html
- Qt QFileSystemWatcher: https://doc.qt.io/qt-6/qfilesystemwatcher.html
- Qt QSaveFile atomic document writes: https://doc.qt.io/qt-6/qsavefile.html
- Qt QLockFile: https://doc.qt.io/qt-6/qlockfile.html
- Qt Quick accessibility: https://doc.qt.io/qt-6/accessible-qtquick.html
- Unity 6 Asset Database refresh/hot reload: https://docs.unity3d.com/6000.0/Documentation/Manual/AssetDatabaseRefreshing.html
- Unity 6 Asset Database: https://docs.unity3d.com/6000.0/Documentation/Manual/AssetDatabase.html
- Unity 6 Asset metadata and stable IDs: https://docs.unity3d.com/6000.0/Documentation/Manual/AssetMetadata.html
- Unity 6 Console: https://docs.unity3d.com/6000.0/Documentation/Manual/Console.html
- SQLite atomic commit: https://www.sqlite.org/atomiccommit.html
- Skia coordinate spaces: https://skia.org/docs/user/coordinates/
- OpenTimelineIO opentime: https://opentimelineio.readthedocs.io/en/v0.17.0/api/python/opentimelineio.opentime.html
- CMake File API: https://cmake.org/cmake/help/latest/manual/cmake-file-api.7.html
- CMake Presets: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- Clang JSON Compilation Database: https://clang.llvm.org/docs/JSONCompilationDatabase.html
- Clang diagnostics reference: https://clang.llvm.org/docs/DiagnosticsReference.html
- Codex AGENTS.md guidance: https://learn.chatgpt.com/docs/agent-configuration/agents-md
- Codex Skills: https://developers.openai.com/plugins/build/skills
- Codex MCP configuration and behavior: https://learn.chatgpt.com/docs/extend/mcp
- Codex non-interactive structured output: https://learn.chatgpt.com/docs/non-interactive-mode
- MCP server primitives: https://modelcontextprotocol.io/specification/2025-06-18/server/index
- MCP Resources and subscriptions: https://modelcontextprotocol.io/specification/2025-11-25/server/resources
- MCP Tools and structured errors: https://modelcontextprotocol.io/specification/2025-06-18/server/tools
- MCP lifecycle/capability negotiation/logging: https://modelcontextprotocol.io/specification/2025-06-18/basic/lifecycle
- FFmpeg hardware contexts: https://www.ffmpeg.org/doxygen/8.0/hwcontext_8h.html
- FFmpeg legal considerations: https://www.ffmpeg.org/legal.html
- Apple VideoToolbox: https://developer.apple.com/documentation/videotoolbox
- Microsoft Media Foundation: https://learn.microsoft.com/en-us/windows/win32/medfound/media-foundation-start-page
- Android MediaCodec: https://developer.android.com/reference/android/media/MediaCodec
- Android AHardwareBuffer: https://developer.android.com/ndk/reference/group/a-hardware-buffer
- OpenColorIO: https://opencolorio.org/
- Adobe Premiere linked audio/video clips: https://helpx.adobe.com/premiere/desktop/add-audio-effects/basic-audio-editing/link-audio-and-video-clips.html
- DaVinci Resolve Fairlight Audio Post guide: https://documents.blackmagicdesign.com/UserManuals/DaVinciResolveFairlightAudioPost.pdf
- FFmpeg libavfilter: https://www.ffmpeg.org/libavfilter.html
- FFmpeg audio filters: https://www.ffmpeg.org/ffmpeg-filters.html
- BBC audiowaveform: https://github.com/bbc/audiowaveform
- JUCE AudioProcessorGraph: https://docs.juce.com/develop/classjuce_1_1AudioProcessorGraph.html
- JUCE licensing: https://juce.com/get-juce/
- VST3 parameters and automation: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Parameters%2BAutomation/Index.html
- EBU R128: https://tech.ebu.ch/publications/r128
- RNNoise: https://github.com/xiph/rnnoise
- DeepFilterNet: https://github.com/Rikorose/DeepFilterNet
- Apple hardware-required VideoToolbox decoder: https://developer.apple.com/documentation/VideoToolbox/kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder
- Apple CVMetalTextureCache live texture binding: https://developer.apple.com/documentation/corevideo/cvmetaltexturecachecreatetexturefromimage%28_%3A_%3A_%3A_%3A_%3A_%3A_%3A_%3A%29
- Apple VideoToolbox/CoreVideo/Metal workflow: https://developer.apple.com/videos/play/wwdc2020/10090/
- Microsoft hardware-only Media Foundation transforms: https://learn.microsoft.com/en-us/windows/win32/medfound/mf-readwrite-use-only-hardware-transforms
- Microsoft D3D12 video decode capability query: https://learn.microsoft.com/en-us/windows/win32/api/d3d12video/ns-d3d12video-d3d12_feature_data_video_decode_support
- Microsoft Media Foundation D3D11 decoder surfaces: https://learn.microsoft.com/en-us/windows/win32/medfound/supporting-direct3d-11-video-decoding-in-media-foundation
- Microsoft Media Foundation D3D12 synchronization: https://learn.microsoft.com/en-us/windows/win32/api/mfd3d12/
- Android MediaCodec hardware/performance queries: https://developer.android.com/media/optimize/performance/codec
- Android MediaCodec Surface mode: https://developer.android.com/reference/android/media/MediaCodec
- Android AHardwareBuffer external memory: https://developer.android.com/ndk/reference/group/a-hardware-buffer
- wgpu external/native texture APIs: https://docs.rs/wgpu/latest/wgpu/struct.Device.html
- Qt native texture interfaces: https://doc.qt.io/qt-6/qnativeinterface.html
- FFmpeg AVStream time base/start/duration: https://ffmpeg.org/doxygen/8.0/structAVStream.html
- FFmpeg AVFrame PTS/duration/audio sample count: https://www.ffmpeg.org/doxygen/trunk/structAVFrame.html
- Apple CMSampleBuffer presentation timestamp: https://developer.apple.com/documentation/coremedia/cmsamplebuffer/presentationtimestamp
- Apple CMSampleBuffer timing information: https://developer.apple.com/documentation/coremedia/cmsampletiminginfo
- Apple AVAudioTime host/sample time: https://developer.apple.com/documentation/avfaudio/avaudiotime
- Microsoft Media Foundation presentation clock: https://learn.microsoft.com/en-us/windows/win32/medfound/presentation-clock
- Microsoft IMFSample time: https://learn.microsoft.com/en-us/windows/win32/api/mfobjects/nf-mfobjects-imfsample-setsampletime
- Android MediaCodec BufferInfo presentation timestamp: https://developer.android.com/reference/android/media/MediaCodec.BufferInfo
- Apple AVAudioTime host/sample clock correlation: https://developer.apple.com/documentation/avfaudio/avaudiotime
- Microsoft IAudioClock device position: https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclock-getposition
- Android AudioTrack device timestamp: https://developer.android.com/reference/android/media/AudioTrack.html#getTimestamp(android.media.AudioTimestamp)
- Android AudioTimestamp frame position/monotonic time: https://developer.android.com/reference/android/media/AudioTimestamp
- OpenUSD time-sampled typed values: https://openusd.org/dev/user_guides/time_and_animated_values.html
- OpenUSD UsdAttribute: https://openusd.org/dev/api/class_usd_attribute.html
- MaterialX typed node specification: https://github.com/AcademySoftwareFoundation/MaterialX/blob/main/documents/Specification/MaterialX.Specification.md
- OpenTimelineIO schema versioning: https://opentimelineio.readthedocs.io/en/v0.16.0/tutorials/versioning-schemas.html
- Qt macOS deployment/macdeployqt: https://doc.qt.io/qt-6/macos-deployment.html
- Qt Windows deployment/windeployqt: https://doc.qt.io/qt-6/windows-deployment.html
- Qt Installer Framework updates: https://doc.qt.io/qtinstallerframework/ifw-updates.html
- Qt LGPL obligations: https://www.qt.io/development/open-source-lgpl-obligations
- Qt licensing overview: https://doc.qt.io/qt-6/licensing.html
- Apple Developer ID notarization: https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution
- Microsoft MSIX signing: https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview
- Microsoft Windows distribution paths: https://learn.microsoft.com/en-us/windows/apps/package-and-deploy/choose-distribution-path
- Microsoft App Installer update/repair: https://learn.microsoft.com/en-us/windows/msix/app-installer/auto-update-and-repair--overview
- Sentry Native: https://github.com/getsentry/sentry-native
- Stripe Entitlements: https://docs.stripe.com/billing/entitlements
- WebAssembly security model: https://webassembly.org/docs/security/
- Wasmtime security: https://docs.wasmtime.dev/security.html
- WebAssembly Interface Types: https://component-model.bytecodealliance.org/design/wit.html
- WASI 0.2: https://wasi.dev/releases/wasi-p2
- GCC C++ interoperability caveats: https://gcc.gnu.org/onlinedocs/gcc/Interoperation.html
- Microsoft C++ binary compatibility: https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017
- OpenFX Core API: https://openfx.readthedocs.io/en/latest/Reference/ofxCoreAPI.html
- The Update Framework specification: https://theupdateframework.github.io/specification/latest/
- Apple App Review Guidelines 2.5.2: https://developer.apple.com/app-store/review/guidelines/
- Google Play Device and Network Abuse: https://support.google.com/googleplay/android-developer/answer/16559646
- Android dynamic code loading risks: https://developer.android.com/privacy-and-security/risks/dynamic-code-loading
- Windows application isolation: https://learn.microsoft.com/en-us/windows/security/book/application-security-application-isolation
- Apple XPC services: https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPSystemStartup/Chapters/CreatingXPCServices.html

---

## 27. سجل تغييرات المسودة

### Draft 0.8 — 2026-08-07

- تحويل تشبيه Unity إلى غربلة `ReFusion Live Authoring Service` من source detection إلى atomic activation.
- فصل Authoring Sources وDerived Artifacts وActive Project Revision وتعريف Source/Artifact generations.
- إضافة Source/Asset Database وDependency Graph وcontent-addressed Artifact Store وLast-Known-Good.
- تعريف Explicit Agent ChangeSet ومسار direct-save reconciliation مع منع watcher event كـtransaction boundary.
- تعريف Revision Presentation Gate و`EvaluationStamp` موحدة لـTimeline وCanvas وInspector.
- تثبيت typed stable IDs للمشروع والـComposition والـTrack والـClip والـLayer والـAsset والـEffect.
- تعريف Pixel-True Canvas وCoordinate Spaces وعقد Rational Time/Frame/Sample وVFR/PTS.
- تعريف Console بشرية و`DiagnosticEnvelope` وRun Bundles يستطيع الأيجنت قراءتها وإصلاحها.
- تصميم Developer Control Plane موحد للـCLI وMCP وGenerated Contracts.
- توزيع دور `AGENTS.md` و`CODEX.md` و`CLOUD.md` وSkills مع progressive disclosure ومنع التخمين.
- توضيح build وShadow Plugin Host لـC++ extensions وفصلها عن Project Authoring وCore restart.
- إضافة التجارب E-061 إلى E-072.

### Draft 0.7 — 2026-08-07

- إضافة غربلة Studio Shell الأصلية بـQt Quick/QML وتوزيع Insert Rail وCanvas/Timeline وInspector.
- تثبيت أن الأيجنت خارجي مثل Codex ولا يوجد Agent button أو chat داخل التطبيق.
- تعريف Project File Ingestor وDisk Candidate وcommit marker وbase revision ومسار الحفظ الذري.
- تعريف Selection كحالة جلسة، وSchema-Driven Inspector، وتكافؤ UI/File semantic digest.
- منع HTML/WebView/Electron وproperty state داخل QML وfilesystem event كحد transaction.
- فصل Project Authoring اللحظي عن Native C++ Extension Development وعن Core Engine Development.
- تعريف Build Orchestrator وstructured diagnostics وside-by-side activation وPlugin Host المعزول وLast Known Good.
- إضافة التجارب E-049 إلى E-060.

### Draft 0.6 — 2026-08-07

- إضافة غربلة Walking Product Skeleton وRelease Spine وNarrow Paid Vertical Slice.
- تصحيح بوابة الـMaster Plan بفصل kill-risk spikes السابقة للخطة عن delivery/conformance gates داخلها.
- اقتراح Creator Loop وDesktop v1/Not‑V1 ونطاق macOS/Windows أولي قابل للقياس.
- ترجيح Unified Layer Entity مع Descriptors/Capabilities وTyped Ports وProperty/Animation System واحدة.
- تعريف Capability Definition of Done التي تربط Command/Revision/UI/Agent/Preview/Export/Persistence/Tests/Packaging.
- إضافة Gate Map من G0 إلى G7، وصولًا إلى Paid Founder Beta وStable v1.
- إدخال signing/notarization/update/rollback/diagnostics/licensing/payment كأجزاء من المنتج منذ البداية.
- ترجيح v1 Plugin-ready لا Public-SDK-ready، مع Wasm Worker لاحق وC++ فوق Stable C ABI داخل Plugin Host منفصل.
- فصل دلالة المشروع المشتركة عن قيود توزيع plugins على iOS/Android.
- إضافة شكل Master Plan المستقبلي وقواعد منع الغوص غير المنتج والتجارب E-033 إلى E-048.

### Draft 0.5 — 2026-08-07

- فصل ProjectTime عن ClockSource وعن Transport Authority وعن Project Scheduler.
- ترجيح ReFusion Project Transport Authority كمالك وحيد لزمن التشغيل داخل Engine Session.
- حصر audio device clock في دور مصدر ticks فعلي، لا مصدر حقيقة أو مالك مشروع.
- تعريف Transport Epoch وanchor mapping والفصل بين Transport State وProject Revision.
- فصل إبطال العمل بواسطة `(transport_epoch_id, accepted_revision_id)` كي لا تعيد التعديلات العادية ساعة التشغيل.
- توضيح priming عند Play ومنع دائرة الاعتماد بين Audio Graph وProjectTime.
- توحيد تقييم audio/video/animation/automation على ProjectTime واحدة.
- إضافة سياسات أولية لـseek وdevice route change والتعديل أثناء التشغيل والـoffline export.
- إضافة التجارب E-028 إلى E-032.

### Draft 0.4 — 2026-08-07

- حصر عقد الزمن في MediaAsset واحدة ومساري الفيديو والصوت المضمّن، لا زمن المشروع كله.
- تعريف Exact Source-Time Semantics وفصلها عن physical A/V presentation tolerance.
- ترجيح rational/integer timing ومنع average-FPS وfloating-point time كمصدر حقيقة.
- إضافة MediaTimingManifest وVideoPresentationIndex وAudioSampleIndex كبيانات مشتقة قابلة لإعادة البناء.
- حفظ stream offsets وVFR وB-frames وaudio priming/padding وedit lists.
- ترجيح audio device/sample clock كمرجع تشغيل لهذا الأصل عند وجود الصوت.
- تثبيت انعدام سلطة UI على PTS وframe selection وaudio sample ranges والـclock.
- إضافة حالات timing الصحة وسياسة fail-closed والتجارب E-023 إلى E-027.

### Draft 0.3 — 2026-08-07

- إضافة غربلة Hardware-Only Native Video Decode على المنصات الأربع.
- تعريف zero-copy بأنها zero CPU pixel copy، مع توضيح حدود GPU passes.
- فصل سلطة UI عن transport/decode/render/presentation بالكامل.
- تسجيل الحاجة إلى bounded engine-owned queues وسبب استحالة منع كل queues.
- إضافة مسارات Apple وWindows وAndroid المرشحة.
- إضافة قائمة صريحة بالمسارات المحظورة وCapability fail-closed.
- إضافة التجارب E-016 إلى E-022.

### Draft 0.2 — 2026-08-07

- إضافة غربلة فصل video/audio إلى component clips مرتبطة.
- إضافة آلية waveform حقيقية متعددة الدقة.
- فصل Clip Studio وTrack/Mixer Studio وBus/Master Studio.
- غربلة FFmpeg/libavfilter وJUCE وGStreamer ومحرك ReFusion Audio Graph.
- إضافة استعادة الصوت والـdenoise والـloudness والمخاطر والاستبعادات.
- إضافة التجارب E-009 إلى E-015.

### Draft 0.1 — 2026-08-07

- جمع المبادئ التي نوقشت حتى الآن.
- فصل مسارات الرسم والـGPU والواجهة والوسائط واللون والـMotion Blur.
- تسجيل الترجيحات الحالية بلا اعتماد.
- تحديد التجارب والمخاطر وبوابة الانتقال إلى الـMaster Plan.
