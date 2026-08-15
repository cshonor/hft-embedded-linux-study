; ModuleID = '08ji9vg2l31l8v6128xlcxn52'
source_filename = "08ji9vg2l31l8v6128xlcxn52"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@vtable.0 = private constant <{ [24 x i8], ptr }> <{ [24 x i8] c"\00\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00", ptr @"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E" }>, align 8, !dbg !0
@vtable.1 = private constant <{ [24 x i8], ptr }> <{ [24 x i8] c"\00\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00", ptr @"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E" }>, align 8, !dbg !20

; llvm_insight_lab::process_dyn
; Function Attrs: noinline uwtable
define i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr align 1 %h.0, ptr align 8 %h.1) unnamed_addr #0 !dbg !38 {
start:
  %h.dbg.spill = alloca [16 x i8], align 8
  store ptr %h.0, ptr %h.dbg.spill, align 8
  %0 = getelementptr inbounds i8, ptr %h.dbg.spill, i64 8
  store ptr %h.1, ptr %0, align 8
    #dbg_declare(ptr %h.dbg.spill, !53, !DIExpression(), !54)
  %1 = getelementptr inbounds i8, ptr %h.1, i64 24, !dbg !55
  %2 = load ptr, ptr %1, align 8, !dbg !55, !invariant.load !8, !nonnull !8
  %_0 = call i64 %2(ptr align 1 %h.0), !dbg !55
  ret i64 %_0, !dbg !56
}

; llvm_insight_lab::process_static
; Function Attrs: noinline uwtable
define i64 @_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE(ptr align 8 %h) unnamed_addr #0 !dbg !57 {
start:
  %h.dbg.spill = alloca [8 x i8], align 8
  store ptr %h, ptr %h.dbg.spill, align 8
    #dbg_declare(ptr %h.dbg.spill, !62, !DIExpression(), !65)
; call <llvm_insight_lab::HandlerB as llvm_insight_lab::TickHandler>::on_tick
  %_0 = call i64 @"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E"(ptr align 8 %h), !dbg !66
  ret i64 %_0, !dbg !67
}

; llvm_insight_lab::process_static
; Function Attrs: noinline uwtable
define i64 @_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E(ptr align 8 %h) unnamed_addr #0 !dbg !68 {
start:
  %h.dbg.spill = alloca [8 x i8], align 8
  store ptr %h, ptr %h.dbg.spill, align 8
    #dbg_declare(ptr %h.dbg.spill, !73, !DIExpression(), !76)
; call <llvm_insight_lab::HandlerA as llvm_insight_lab::TickHandler>::on_tick
  %_0 = call i64 @"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E"(ptr align 8 %h), !dbg !77
  ret i64 %_0, !dbg !78
}

; llvm_insight_lab::demo_dyn_dispatch
; Function Attrs: noinline uwtable
define i64 @_ZN16llvm_insight_lab17demo_dyn_dispatch17h63ff130f0038c151E() unnamed_addr #0 !dbg !79 {
start:
  %rhs.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill.i = alloca [8 x i8], align 8
  %hb.dbg.spill = alloca [16 x i8], align 8
  %ha.dbg.spill = alloca [16 x i8], align 8
  %b = alloca [8 x i8], align 8
  %a = alloca [8 x i8], align 8
    #dbg_declare(ptr %a, !83, !DIExpression(), !91)
    #dbg_declare(ptr %b, !85, !DIExpression(), !92)
  store i64 10, ptr %a, align 8, !dbg !93
  store i64 20, ptr %b, align 8, !dbg !94
  store ptr %a, ptr %ha.dbg.spill, align 8, !dbg !95
  %0 = getelementptr inbounds i8, ptr %ha.dbg.spill, i64 8, !dbg !95
  store ptr @vtable.0, ptr %0, align 8, !dbg !95
    #dbg_declare(ptr %ha.dbg.spill, !87, !DIExpression(), !96)
  store ptr %b, ptr %hb.dbg.spill, align 8, !dbg !97
  %1 = getelementptr inbounds i8, ptr %hb.dbg.spill, i64 8, !dbg !97
  store ptr @vtable.1, ptr %1, align 8, !dbg !97
    #dbg_declare(ptr %hb.dbg.spill, !89, !DIExpression(), !98)
; call llvm_insight_lab::process_dyn
  %_7 = call i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr align 1 %a, ptr align 8 @vtable.0), !dbg !99
; call llvm_insight_lab::process_dyn
  %_8 = call i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr align 1 %b, ptr align 8 @vtable.1), !dbg !99
  store i64 %_7, ptr %self.dbg.spill.i, align 8
    #dbg_declare(ptr %self.dbg.spill.i, !100, !DIExpression(), !110)
  store i64 %_8, ptr %rhs.dbg.spill.i, align 8
    #dbg_declare(ptr %rhs.dbg.spill.i, !109, !DIExpression(), !110)
  %_0.i = add i64 %_7, %_8, !dbg !112
  ret i64 %_0.i, !dbg !113
}

; llvm_insight_lab::demo_static_dispatch
; Function Attrs: noinline uwtable
define i64 @_ZN16llvm_insight_lab20demo_static_dispatch17heb9c55b3ed6042e3E() unnamed_addr #0 !dbg !114 {
start:
  %rhs.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill.i = alloca [8 x i8], align 8
  %b = alloca [8 x i8], align 8
  %a = alloca [8 x i8], align 8
    #dbg_declare(ptr %a, !116, !DIExpression(), !120)
    #dbg_declare(ptr %b, !118, !DIExpression(), !121)
  store i64 10, ptr %a, align 8, !dbg !122
  store i64 20, ptr %b, align 8, !dbg !123
; call llvm_insight_lab::process_static
  %_3 = call i64 @_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E(ptr align 8 %a), !dbg !124
; call llvm_insight_lab::process_static
  %_5 = call i64 @_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE(ptr align 8 %b), !dbg !124
  store i64 %_3, ptr %self.dbg.spill.i, align 8
    #dbg_declare(ptr %self.dbg.spill.i, !100, !DIExpression(), !125)
  store i64 %_5, ptr %rhs.dbg.spill.i, align 8
    #dbg_declare(ptr %rhs.dbg.spill.i, !109, !DIExpression(), !125)
  %_0.i = add i64 %_3, %_5, !dbg !127
  ret i64 %_0.i, !dbg !128
}

; <llvm_insight_lab::HandlerA as llvm_insight_lab::TickHandler>::on_tick
; Function Attrs: uwtable
define i64 @"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E"(ptr align 8 %self) unnamed_addr #1 !dbg !129 {
start:
  %rhs.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill = alloca [8 x i8], align 8
  store ptr %self, ptr %self.dbg.spill, align 8
    #dbg_declare(ptr %self.dbg.spill, !132, !DIExpression(), !133)
  %_2 = load i64, ptr %self, align 8, !dbg !134
  store i64 %_2, ptr %self.dbg.spill.i, align 8
    #dbg_declare(ptr %self.dbg.spill.i, !100, !DIExpression(), !135)
  store i64 1, ptr %rhs.dbg.spill.i, align 8
    #dbg_declare(ptr %rhs.dbg.spill.i, !109, !DIExpression(), !135)
  %_0.i = add i64 %_2, 1, !dbg !137
  ret i64 %_0.i, !dbg !138
}

; <llvm_insight_lab::HandlerB as llvm_insight_lab::TickHandler>::on_tick
; Function Attrs: uwtable
define i64 @"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E"(ptr align 8 %self) unnamed_addr #1 !dbg !139 {
start:
  %rhs.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill.i = alloca [8 x i8], align 8
  %self.dbg.spill = alloca [8 x i8], align 8
  store ptr %self, ptr %self.dbg.spill, align 8
    #dbg_declare(ptr %self.dbg.spill, !142, !DIExpression(), !143)
  %_2 = load i64, ptr %self, align 8, !dbg !144
  store i64 %_2, ptr %self.dbg.spill.i, align 8
    #dbg_declare(ptr %self.dbg.spill.i, !145, !DIExpression(), !149)
  store i64 2, ptr %rhs.dbg.spill.i, align 8
    #dbg_declare(ptr %rhs.dbg.spill.i, !148, !DIExpression(), !149)
  %_0.i = mul i64 %_2, 2, !dbg !151
  ret i64 %_0.i, !dbg !152
}

attributes #0 = { noinline uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }
attributes #1 = { uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }

!llvm.module.flags = !{!31, !32, !33}
!llvm.ident = !{!34}
!llvm.dbg.cu = !{!35}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "impl$<llvm_insight_lab::HandlerA, llvm_insight_lab::TickHandler>::vtable$", scope: null, file: !2, type: !3, isLocal: true, isDefinition: true)
!2 = !DIFile(filename: "<unknown>", directory: "")
!3 = !DICompositeType(tag: DW_TAG_structure_type, name: "impl$<llvm_insight_lab::HandlerA, llvm_insight_lab::TickHandler>::vtable_type$", file: !2, size: 256, align: 64, flags: DIFlagArtificial, elements: !4, vtableHolder: !14, templateParams: !8, identifier: "92c032024a7a6a6c643dcc64e43fd908")
!4 = !{!5, !9, !12, !13}
!5 = !DIDerivedType(tag: DW_TAG_member, name: "drop_in_place", scope: !3, file: !2, baseType: !6, size: 64, align: 64)
!6 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ptr_const$<tuple$<> >", baseType: !7, size: 64, align: 64, dwarfAddressSpace: 0)
!7 = !DICompositeType(tag: DW_TAG_structure_type, name: "tuple$<>", file: !2, align: 8, elements: !8, identifier: "f7c8c55ec2a6a300648f1e0cb113054e")
!8 = !{}
!9 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !3, file: !2, baseType: !10, size: 64, align: 64, offset: 64)
!10 = !DIDerivedType(tag: DW_TAG_typedef, name: "usize", file: !2, baseType: !11)
!11 = !DIBasicType(name: "size_t", size: 64, encoding: DW_ATE_unsigned)
!12 = !DIDerivedType(tag: DW_TAG_member, name: "align", scope: !3, file: !2, baseType: !10, size: 64, align: 64, offset: 128)
!13 = !DIDerivedType(tag: DW_TAG_member, name: "__method3", scope: !3, file: !2, baseType: !6, size: 64, align: 64, offset: 192)
!14 = !DICompositeType(tag: DW_TAG_structure_type, name: "HandlerA", scope: !15, file: !2, size: 64, align: 64, flags: DIFlagPublic, elements: !16, templateParams: !8, identifier: "63734491c5ed13419c2f02cb7f9f441a")
!15 = !DINamespace(name: "llvm_insight_lab", scope: null)
!16 = !{!17}
!17 = !DIDerivedType(tag: DW_TAG_member, name: "__0", scope: !14, file: !2, baseType: !18, size: 64, align: 64, flags: DIFlagPublic)
!18 = !DIDerivedType(tag: DW_TAG_typedef, name: "u64", file: !2, baseType: !19)
!19 = !DIBasicType(name: "unsigned __int64", size: 64, encoding: DW_ATE_unsigned)
!20 = !DIGlobalVariableExpression(var: !21, expr: !DIExpression())
!21 = distinct !DIGlobalVariable(name: "impl$<llvm_insight_lab::HandlerB, llvm_insight_lab::TickHandler>::vtable$", scope: null, file: !2, type: !22, isLocal: true, isDefinition: true)
!22 = !DICompositeType(tag: DW_TAG_structure_type, name: "impl$<llvm_insight_lab::HandlerB, llvm_insight_lab::TickHandler>::vtable_type$", file: !2, size: 256, align: 64, flags: DIFlagArtificial, elements: !23, vtableHolder: !28, templateParams: !8, identifier: "e7aabf7cfa1e7e3e986b5314674d85ed")
!23 = !{!24, !25, !26, !27}
!24 = !DIDerivedType(tag: DW_TAG_member, name: "drop_in_place", scope: !22, file: !2, baseType: !6, size: 64, align: 64)
!25 = !DIDerivedType(tag: DW_TAG_member, name: "size", scope: !22, file: !2, baseType: !10, size: 64, align: 64, offset: 64)
!26 = !DIDerivedType(tag: DW_TAG_member, name: "align", scope: !22, file: !2, baseType: !10, size: 64, align: 64, offset: 128)
!27 = !DIDerivedType(tag: DW_TAG_member, name: "__method3", scope: !22, file: !2, baseType: !6, size: 64, align: 64, offset: 192)
!28 = !DICompositeType(tag: DW_TAG_structure_type, name: "HandlerB", scope: !15, file: !2, size: 64, align: 64, flags: DIFlagPublic, elements: !29, templateParams: !8, identifier: "69d38155713f1c16b7c14a49520d380e")
!29 = !{!30}
!30 = !DIDerivedType(tag: DW_TAG_member, name: "__0", scope: !28, file: !2, baseType: !18, size: 64, align: 64, flags: DIFlagPublic)
!31 = !{i32 8, !"PIC Level", i32 2}
!32 = !{i32 2, !"CodeView", i32 1}
!33 = !{i32 2, !"Debug Info Version", i32 3}
!34 = !{!"rustc version 1.93.0-nightly (e65b98316 2025-11-15)"}
!35 = distinct !DICompileUnit(language: DW_LANG_Rust, file: !36, producer: "clang LLVM (rustc version 1.93.0-nightly (e65b98316 2025-11-15))", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, globals: !37, splitDebugInlining: false)
!36 = !DIFile(filename: "src\\lib.rs\\@\\08ji9vg2l31l8v6128xlcxn52", directory: "C:\\Users\\12392\\Desktop\\rs\\rs study\\llvm_insight")
!37 = !{!0, !20}
!38 = distinct !DISubprogram(name: "process_dyn", linkageName: "_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E", scope: !15, file: !39, line: 38, type: !40, scopeLine: 38, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !52)
!39 = !DIFile(filename: "src\\lib.rs", directory: "C:\\Users\\12392\\Desktop\\rs\\rs study\\llvm_insight", checksumkind: CSK_SHA256, checksum: "31d657dc537fb72ffe4c9cb30d94d05cfec9f3c7ddf0d81236f534bced55b2d5")
!40 = !DISubroutineType(types: !41)
!41 = !{!18, !42}
!42 = !DICompositeType(tag: DW_TAG_structure_type, name: "ref$<dyn$<llvm_insight_lab::TickHandler> >", file: !2, size: 128, align: 64, elements: !43, templateParams: !8, identifier: "882fa0502f23b16adf411f097acd2cae")
!43 = !{!44, !47}
!44 = !DIDerivedType(tag: DW_TAG_member, name: "pointer", scope: !42, file: !2, baseType: !45, size: 64, align: 64)
!45 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !46, size: 64, align: 64, dwarfAddressSpace: 0)
!46 = !DICompositeType(tag: DW_TAG_structure_type, name: "dyn$<llvm_insight_lab::TickHandler>", file: !2, align: 8, elements: !8, identifier: "9b6d9b0efcdaec52ca8e5fe43ed957c1")
!47 = !DIDerivedType(tag: DW_TAG_member, name: "vtable", scope: !42, file: !2, baseType: !48, size: 64, align: 64, offset: 64)
!48 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ref$<array$<usize,4> >", baseType: !49, size: 64, align: 64, dwarfAddressSpace: 0)
!49 = !DICompositeType(tag: DW_TAG_array_type, baseType: !10, size: 256, align: 64, elements: !50)
!50 = !{!51}
!51 = !DISubrange(count: 4, lowerBound: 0)
!52 = !{!53}
!53 = !DILocalVariable(name: "h", scope: !38, file: !39, line: 38, type: !42, align: 64)
!54 = !DILocation(line: 38, scope: !38)
!55 = !DILocation(line: 39, scope: !38)
!56 = !DILocation(line: 40, scope: !38)
!57 = distinct !DISubprogram(name: "process_static<llvm_insight_lab::HandlerB>", linkageName: "_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE", scope: !15, file: !39, line: 32, type: !58, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !35, templateParams: !63, retainedNodes: !61)
!58 = !DISubroutineType(types: !59)
!59 = !{!18, !60}
!60 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ref$<llvm_insight_lab::HandlerB>", baseType: !28, size: 64, align: 64, dwarfAddressSpace: 0)
!61 = !{!62}
!62 = !DILocalVariable(name: "h", arg: 1, scope: !57, file: !39, line: 32, type: !60)
!63 = !{!64}
!64 = !DITemplateTypeParameter(name: "T", type: !28)
!65 = !DILocation(line: 32, scope: !57)
!66 = !DILocation(line: 33, scope: !57)
!67 = !DILocation(line: 34, scope: !57)
!68 = distinct !DISubprogram(name: "process_static<llvm_insight_lab::HandlerA>", linkageName: "_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E", scope: !15, file: !39, line: 32, type: !69, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !35, templateParams: !74, retainedNodes: !72)
!69 = !DISubroutineType(types: !70)
!70 = !{!18, !71}
!71 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ref$<llvm_insight_lab::HandlerA>", baseType: !14, size: 64, align: 64, dwarfAddressSpace: 0)
!72 = !{!73}
!73 = !DILocalVariable(name: "h", arg: 1, scope: !68, file: !39, line: 32, type: !71)
!74 = !{!75}
!75 = !DITemplateTypeParameter(name: "T", type: !14)
!76 = !DILocation(line: 32, scope: !68)
!77 = !DILocation(line: 33, scope: !68)
!78 = !DILocation(line: 34, scope: !68)
!79 = distinct !DISubprogram(name: "demo_dyn_dispatch", linkageName: "_ZN16llvm_insight_lab17demo_dyn_dispatch17h63ff130f0038c151E", scope: !15, file: !39, line: 50, type: !80, scopeLine: 50, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !82)
!80 = !DISubroutineType(types: !81)
!81 = !{!18}
!82 = !{!83, !85, !87, !89}
!83 = !DILocalVariable(name: "a", scope: !84, file: !39, line: 51, type: !14, align: 64)
!84 = distinct !DILexicalBlock(scope: !79, file: !39, line: 51)
!85 = !DILocalVariable(name: "b", scope: !86, file: !39, line: 52, type: !28, align: 64)
!86 = distinct !DILexicalBlock(scope: !84, file: !39, line: 52)
!87 = !DILocalVariable(name: "ha", scope: !88, file: !39, line: 53, type: !42, align: 64)
!88 = distinct !DILexicalBlock(scope: !86, file: !39, line: 53)
!89 = !DILocalVariable(name: "hb", scope: !90, file: !39, line: 54, type: !42, align: 64)
!90 = distinct !DILexicalBlock(scope: !88, file: !39, line: 54)
!91 = !DILocation(line: 51, scope: !84)
!92 = !DILocation(line: 52, scope: !86)
!93 = !DILocation(line: 51, scope: !79)
!94 = !DILocation(line: 52, scope: !84)
!95 = !DILocation(line: 53, scope: !86)
!96 = !DILocation(line: 53, scope: !88)
!97 = !DILocation(line: 54, scope: !88)
!98 = !DILocation(line: 54, scope: !90)
!99 = !DILocation(line: 55, scope: !90)
!100 = !DILocalVariable(name: "self", arg: 1, scope: !101, file: !102, line: 2300, type: !18)
!101 = distinct !DISubprogram(name: "wrapping_add", linkageName: "_ZN4core3num21_$LT$impl$u20$u64$GT$12wrapping_add17h416c53aa21deb477E", scope: !103, file: !102, line: 2300, type: !106, scopeLine: 2300, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !108)
!102 = !DIFile(filename: "C:\\Users\\12392\\.rustup\\toolchains\\nightly-x86_64-pc-windows-msvc\\lib/rustlib/src/rust\\library\\core\\src\\num\\uint_macros.rs", directory: "", checksumkind: CSK_SHA256, checksum: "1a2ca4eb79c76955875c5c048def86a990353e6d83d54e43324ed9ea05141777")
!103 = !DINamespace(name: "impl$9", scope: !104)
!104 = !DINamespace(name: "num", scope: !105)
!105 = !DINamespace(name: "core", scope: null)
!106 = !DISubroutineType(types: !107)
!107 = !{!18, !18, !18}
!108 = !{!100, !109}
!109 = !DILocalVariable(name: "rhs", arg: 2, scope: !101, file: !102, line: 2300, type: !18)
!110 = !DILocation(line: 2300, scope: !101, inlinedAt: !111)
!111 = distinct !DILocation(line: 55, scope: !90)
!112 = !DILocation(line: 2301, scope: !101, inlinedAt: !111)
!113 = !DILocation(line: 56, scope: !79)
!114 = distinct !DISubprogram(name: "demo_static_dispatch", linkageName: "_ZN16llvm_insight_lab20demo_static_dispatch17heb9c55b3ed6042e3E", scope: !15, file: !39, line: 43, type: !80, scopeLine: 43, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !115)
!115 = !{!116, !118}
!116 = !DILocalVariable(name: "a", scope: !117, file: !39, line: 44, type: !14, align: 64)
!117 = distinct !DILexicalBlock(scope: !114, file: !39, line: 44)
!118 = !DILocalVariable(name: "b", scope: !119, file: !39, line: 45, type: !28, align: 64)
!119 = distinct !DILexicalBlock(scope: !117, file: !39, line: 45)
!120 = !DILocation(line: 44, scope: !117)
!121 = !DILocation(line: 45, scope: !119)
!122 = !DILocation(line: 44, scope: !114)
!123 = !DILocation(line: 45, scope: !117)
!124 = !DILocation(line: 46, scope: !119)
!125 = !DILocation(line: 2300, scope: !101, inlinedAt: !126)
!126 = distinct !DILocation(line: 46, scope: !119)
!127 = !DILocation(line: 2301, scope: !101, inlinedAt: !126)
!128 = !DILocation(line: 47, scope: !114)
!129 = distinct !DISubprogram(name: "on_tick", linkageName: "_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E", scope: !130, file: !39, line: 19, type: !69, scopeLine: 19, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !131)
!130 = !DINamespace(name: "impl$0", scope: !15)
!131 = !{!132}
!132 = !DILocalVariable(name: "self", arg: 1, scope: !129, file: !39, line: 19, type: !71)
!133 = !DILocation(line: 19, scope: !129)
!134 = !DILocation(line: 20, scope: !129)
!135 = !DILocation(line: 2300, scope: !101, inlinedAt: !136)
!136 = distinct !DILocation(line: 20, scope: !129)
!137 = !DILocation(line: 2301, scope: !101, inlinedAt: !136)
!138 = !DILocation(line: 21, scope: !129)
!139 = distinct !DISubprogram(name: "on_tick", linkageName: "_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E", scope: !140, file: !39, line: 25, type: !58, scopeLine: 25, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !141)
!140 = !DINamespace(name: "impl$1", scope: !15)
!141 = !{!142}
!142 = !DILocalVariable(name: "self", arg: 1, scope: !139, file: !39, line: 25, type: !60)
!143 = !DILocation(line: 25, scope: !139)
!144 = !DILocation(line: 26, scope: !139)
!145 = !DILocalVariable(name: "self", arg: 1, scope: !146, file: !102, line: 2376, type: !18)
!146 = distinct !DISubprogram(name: "wrapping_mul", linkageName: "_ZN4core3num21_$LT$impl$u20$u64$GT$12wrapping_mul17ha6c516387cf277b5E", scope: !103, file: !102, line: 2376, type: !106, scopeLine: 2376, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition, unit: !35, templateParams: !8, retainedNodes: !147)
!147 = !{!145, !148}
!148 = !DILocalVariable(name: "rhs", arg: 2, scope: !146, file: !102, line: 2376, type: !18)
!149 = !DILocation(line: 2376, scope: !146, inlinedAt: !150)
!150 = distinct !DILocation(line: 26, scope: !139)
!151 = !DILocation(line: 2377, scope: !146, inlinedAt: !150)
!152 = !DILocation(line: 27, scope: !139)
