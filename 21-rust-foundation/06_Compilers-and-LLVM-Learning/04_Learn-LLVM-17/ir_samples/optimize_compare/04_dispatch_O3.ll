; ModuleID = '08ji9vg2l31l8v6128xlcxn52'
source_filename = "08ji9vg2l31l8v6128xlcxn52"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-windows-msvc"

@vtable.0 = private constant <{ [24 x i8], ptr }> <{ [24 x i8] c"\00\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00", ptr @"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E" }>, align 8, !dbg !0
@vtable.1 = private constant <{ [24 x i8], ptr }> <{ [24 x i8] c"\00\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00\08\00\00\00\00\00\00\00", ptr @"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E" }>, align 8, !dbg !20

; llvm_insight_lab::process_dyn
; Function Attrs: noinline uwtable
define noundef i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr noundef nonnull align 1 %h.0, ptr noalias noundef readonly align 8 captures(none) dereferenceable(32) %h.1) unnamed_addr #0 !dbg !38 {
start:
    #dbg_value(ptr %h.0, !53, !DIExpression(DW_OP_LLVM_fragment, 0, 64), !54)
    #dbg_value(ptr %h.1, !53, !DIExpression(DW_OP_LLVM_fragment, 64, 64), !54)
  %0 = getelementptr inbounds nuw i8, ptr %h.1, i64 24, !dbg !55
  %1 = load ptr, ptr %0, align 8, !dbg !55, !invariant.load !8, !nonnull !8
  %_0 = tail call noundef i64 %1(ptr noundef nonnull align 1 %h.0), !dbg !55
  ret i64 %_0, !dbg !56
}

; llvm_insight_lab::process_static
; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define noundef range(i64 0, -1) i64 @_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE(ptr noalias noundef readonly align 8 captures(none) dereferenceable(8) %h) unnamed_addr #1 !dbg !57 {
start:
    #dbg_value(ptr %h, !62, !DIExpression(), !65)
    #dbg_value(ptr %h, !66, !DIExpression(), !70)
  %_2.i = load i64, ptr %h, align 8, !dbg !72, !alias.scope !73, !noundef !8
    #dbg_value(i64 %_2.i, !76, !DIExpression(), !86)
    #dbg_value(i64 2, !85, !DIExpression(), !86)
  %_0.i.i = shl i64 %_2.i, 1, !dbg !88
  ret i64 %_0.i.i, !dbg !89
}

; llvm_insight_lab::process_static
; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define noundef i64 @_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E(ptr noalias noundef readonly align 8 captures(none) dereferenceable(8) %h) unnamed_addr #1 !dbg !90 {
start:
    #dbg_value(ptr %h, !95, !DIExpression(), !98)
    #dbg_value(ptr %h, !99, !DIExpression(), !103)
  %_2.i = load i64, ptr %h, align 8, !dbg !105, !alias.scope !106, !noundef !8
    #dbg_value(i64 %_2.i, !109, !DIExpression(), !113)
    #dbg_value(i64 1, !112, !DIExpression(), !113)
  %_0.i.i = add i64 %_2.i, 1, !dbg !115
  ret i64 %_0.i.i, !dbg !116
}

; llvm_insight_lab::demo_dyn_dispatch
; Function Attrs: noinline uwtable
define noundef i64 @_ZN16llvm_insight_lab17demo_dyn_dispatch17h63ff130f0038c151E() unnamed_addr #0 !dbg !117 {
start:
  %b = alloca [8 x i8], align 8
  %a = alloca [8 x i8], align 8
    #dbg_declare(ptr %a, !121, !DIExpression(), !129)
    #dbg_declare(ptr %b, !123, !DIExpression(), !130)
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %a), !dbg !131
  store i64 10, ptr %a, align 8, !dbg !131
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %b), !dbg !132
  store i64 20, ptr %b, align 8, !dbg !132
    #dbg_value(ptr %a, !125, !DIExpression(DW_OP_LLVM_fragment, 0, 64), !133)
    #dbg_value(ptr @vtable.0, !125, !DIExpression(DW_OP_LLVM_fragment, 64, 64), !133)
    #dbg_value(ptr %b, !127, !DIExpression(DW_OP_LLVM_fragment, 0, 64), !134)
    #dbg_value(ptr @vtable.1, !127, !DIExpression(DW_OP_LLVM_fragment, 64, 64), !134)
; call llvm_insight_lab::process_dyn
  %_7 = call noundef i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr noundef nonnull align 1 %a, ptr noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(32) @vtable.0), !dbg !135
; call llvm_insight_lab::process_dyn
  %_8 = call noundef i64 @_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E(ptr noundef nonnull align 1 %b, ptr noalias noundef readonly align 8 captures(address, read_provenance) dereferenceable(32) @vtable.1), !dbg !135
    #dbg_value(i64 %_7, !109, !DIExpression(), !136)
    #dbg_value(i64 %_8, !112, !DIExpression(), !136)
  %_0.i = add i64 %_8, %_7, !dbg !138
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %b), !dbg !139
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %a), !dbg !140
  ret i64 %_0.i, !dbg !140
}

; llvm_insight_lab::demo_static_dispatch
; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define noundef i64 @_ZN16llvm_insight_lab20demo_static_dispatch17heb9c55b3ed6042e3E() unnamed_addr #2 !dbg !141 {
start:
  %b = alloca [8 x i8], align 8
  %a = alloca [8 x i8], align 8
    #dbg_declare(ptr %a, !143, !DIExpression(), !147)
    #dbg_declare(ptr %b, !145, !DIExpression(), !148)
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %a), !dbg !149
  store i64 10, ptr %a, align 8, !dbg !149
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %b), !dbg !150
  store i64 20, ptr %b, align 8, !dbg !150
; call llvm_insight_lab::process_static
  %_3 = call noundef i64 @_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E(ptr noalias noundef nonnull readonly align 8 captures(address, read_provenance) dereferenceable(8) %a), !dbg !151
; call llvm_insight_lab::process_static
  %_5 = call noundef i64 @_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE(ptr noalias noundef nonnull readonly align 8 captures(address, read_provenance) dereferenceable(8) %b), !dbg !151
    #dbg_value(i64 %_3, !109, !DIExpression(), !152)
    #dbg_value(i64 %_5, !112, !DIExpression(), !152)
  %_0.i = add i64 %_5, %_3, !dbg !154
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %b), !dbg !155
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %a), !dbg !156
  ret i64 %_0.i, !dbg !156
}

; <llvm_insight_lab::HandlerA as llvm_insight_lab::TickHandler>::on_tick
; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define noundef i64 @"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E"(ptr noalias noundef readonly align 8 captures(none) dereferenceable(8) %self) unnamed_addr #3 !dbg !100 {
start:
    #dbg_value(ptr %self, !99, !DIExpression(), !157)
  %_2 = load i64, ptr %self, align 8, !dbg !158, !noundef !8
    #dbg_value(i64 %_2, !109, !DIExpression(), !159)
    #dbg_value(i64 1, !112, !DIExpression(), !159)
  %_0.i = add i64 %_2, 1, !dbg !161
  ret i64 %_0.i, !dbg !162
}

; <llvm_insight_lab::HandlerB as llvm_insight_lab::TickHandler>::on_tick
; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable
define noundef range(i64 0, -1) i64 @"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E"(ptr noalias noundef readonly align 8 captures(none) dereferenceable(8) %self) unnamed_addr #3 !dbg !67 {
start:
    #dbg_value(ptr %self, !66, !DIExpression(), !163)
  %_2 = load i64, ptr %self, align 8, !dbg !164, !noundef !8
    #dbg_value(i64 %_2, !76, !DIExpression(), !165)
    #dbg_value(i64 2, !85, !DIExpression(), !165)
  %_0.i = shl i64 %_2, 1, !dbg !167
  ret i64 %_0.i, !dbg !168
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr captures(none)) #4

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr captures(none)) #4

attributes #0 = { noinline uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }
attributes #1 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(argmem: read) uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }
attributes #2 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }
attributes #3 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: read) uwtable "target-cpu"="x86-64" "target-features"="+cx16,+sse3,+sahf" }
attributes #4 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }

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
!35 = distinct !DICompileUnit(language: DW_LANG_Rust, file: !36, producer: "clang LLVM (rustc version 1.93.0-nightly (e65b98316 2025-11-15))", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug, globals: !37, splitDebugInlining: false)
!36 = !DIFile(filename: "src\\lib.rs\\@\\08ji9vg2l31l8v6128xlcxn52", directory: "C:\\Users\\12392\\Desktop\\rs\\rs study\\llvm_insight")
!37 = !{!0, !20}
!38 = distinct !DISubprogram(name: "process_dyn", linkageName: "_ZN16llvm_insight_lab11process_dyn17h779f6621e4f4eed1E", scope: !15, file: !39, line: 38, type: !40, scopeLine: 38, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !52)
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
!54 = !DILocation(line: 0, scope: !38)
!55 = !DILocation(line: 39, scope: !38)
!56 = !DILocation(line: 40, scope: !38)
!57 = distinct !DISubprogram(name: "process_static<llvm_insight_lab::HandlerB>", linkageName: "_ZN16llvm_insight_lab14process_static17h9130f767c2c9e6adE", scope: !15, file: !39, line: 32, type: !58, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !63, retainedNodes: !61)
!58 = !DISubroutineType(types: !59)
!59 = !{!18, !60}
!60 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ref$<llvm_insight_lab::HandlerB>", baseType: !28, size: 64, align: 64, dwarfAddressSpace: 0)
!61 = !{!62}
!62 = !DILocalVariable(name: "h", arg: 1, scope: !57, file: !39, line: 32, type: !60)
!63 = !{!64}
!64 = !DITemplateTypeParameter(name: "T", type: !28)
!65 = !DILocation(line: 0, scope: !57)
!66 = !DILocalVariable(name: "self", arg: 1, scope: !67, file: !39, line: 25, type: !60)
!67 = distinct !DISubprogram(name: "on_tick", linkageName: "_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E", scope: !68, file: !39, line: 25, type: !58, scopeLine: 25, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !69)
!68 = !DINamespace(name: "impl$1", scope: !15)
!69 = !{!66}
!70 = !DILocation(line: 0, scope: !67, inlinedAt: !71)
!71 = distinct !DILocation(line: 33, scope: !57)
!72 = !DILocation(line: 26, scope: !67, inlinedAt: !71)
!73 = !{!74}
!74 = distinct !{!74, !75, !"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E: %self"}
!75 = distinct !{!75, !"_ZN76_$LT$llvm_insight_lab..HandlerB$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hdb63fda1ea3bd5c5E"}
!76 = !DILocalVariable(name: "self", arg: 1, scope: !77, file: !78, line: 2376, type: !18)
!77 = distinct !DISubprogram(name: "wrapping_mul", linkageName: "_ZN4core3num21_$LT$impl$u20$u64$GT$12wrapping_mul17ha6c516387cf277b5E", scope: !79, file: !78, line: 2376, type: !82, scopeLine: 2376, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !84)
!78 = !DIFile(filename: "C:\\Users\\12392\\.rustup\\toolchains\\nightly-x86_64-pc-windows-msvc\\lib/rustlib/src/rust\\library\\core\\src\\num\\uint_macros.rs", directory: "", checksumkind: CSK_SHA256, checksum: "1a2ca4eb79c76955875c5c048def86a990353e6d83d54e43324ed9ea05141777")
!79 = !DINamespace(name: "impl$9", scope: !80)
!80 = !DINamespace(name: "num", scope: !81)
!81 = !DINamespace(name: "core", scope: null)
!82 = !DISubroutineType(types: !83)
!83 = !{!18, !18, !18}
!84 = !{!76, !85}
!85 = !DILocalVariable(name: "rhs", arg: 2, scope: !77, file: !78, line: 2376, type: !18)
!86 = !DILocation(line: 0, scope: !77, inlinedAt: !87)
!87 = distinct !DILocation(line: 26, scope: !67, inlinedAt: !71)
!88 = !DILocation(line: 2377, scope: !77, inlinedAt: !87)
!89 = !DILocation(line: 34, scope: !57)
!90 = distinct !DISubprogram(name: "process_static<llvm_insight_lab::HandlerA>", linkageName: "_ZN16llvm_insight_lab14process_static17hccf3183a43b399d2E", scope: !15, file: !39, line: 32, type: !91, scopeLine: 32, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !96, retainedNodes: !94)
!91 = !DISubroutineType(types: !92)
!92 = !{!18, !93}
!93 = !DIDerivedType(tag: DW_TAG_pointer_type, name: "ref$<llvm_insight_lab::HandlerA>", baseType: !14, size: 64, align: 64, dwarfAddressSpace: 0)
!94 = !{!95}
!95 = !DILocalVariable(name: "h", arg: 1, scope: !90, file: !39, line: 32, type: !93)
!96 = !{!97}
!97 = !DITemplateTypeParameter(name: "T", type: !14)
!98 = !DILocation(line: 0, scope: !90)
!99 = !DILocalVariable(name: "self", arg: 1, scope: !100, file: !39, line: 19, type: !93)
!100 = distinct !DISubprogram(name: "on_tick", linkageName: "_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E", scope: !101, file: !39, line: 19, type: !91, scopeLine: 19, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !102)
!101 = !DINamespace(name: "impl$0", scope: !15)
!102 = !{!99}
!103 = !DILocation(line: 0, scope: !100, inlinedAt: !104)
!104 = distinct !DILocation(line: 33, scope: !90)
!105 = !DILocation(line: 20, scope: !100, inlinedAt: !104)
!106 = !{!107}
!107 = distinct !{!107, !108, !"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E: %self"}
!108 = distinct !{!108, !"_ZN76_$LT$llvm_insight_lab..HandlerA$u20$as$u20$llvm_insight_lab..TickHandler$GT$7on_tick17hd6847b72be4f6660E"}
!109 = !DILocalVariable(name: "self", arg: 1, scope: !110, file: !78, line: 2300, type: !18)
!110 = distinct !DISubprogram(name: "wrapping_add", linkageName: "_ZN4core3num21_$LT$impl$u20$u64$GT$12wrapping_add17h416c53aa21deb477E", scope: !79, file: !78, line: 2300, type: !82, scopeLine: 2300, flags: DIFlagPrototyped, spFlags: DISPFlagLocalToUnit | DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !111)
!111 = !{!109, !112}
!112 = !DILocalVariable(name: "rhs", arg: 2, scope: !110, file: !78, line: 2300, type: !18)
!113 = !DILocation(line: 0, scope: !110, inlinedAt: !114)
!114 = distinct !DILocation(line: 20, scope: !100, inlinedAt: !104)
!115 = !DILocation(line: 2301, scope: !110, inlinedAt: !114)
!116 = !DILocation(line: 34, scope: !90)
!117 = distinct !DISubprogram(name: "demo_dyn_dispatch", linkageName: "_ZN16llvm_insight_lab17demo_dyn_dispatch17h63ff130f0038c151E", scope: !15, file: !39, line: 50, type: !118, scopeLine: 50, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !120)
!118 = !DISubroutineType(types: !119)
!119 = !{!18}
!120 = !{!121, !123, !125, !127}
!121 = !DILocalVariable(name: "a", scope: !122, file: !39, line: 51, type: !14, align: 64)
!122 = distinct !DILexicalBlock(scope: !117, file: !39, line: 51)
!123 = !DILocalVariable(name: "b", scope: !124, file: !39, line: 52, type: !28, align: 64)
!124 = distinct !DILexicalBlock(scope: !122, file: !39, line: 52)
!125 = !DILocalVariable(name: "ha", scope: !126, file: !39, line: 53, type: !42, align: 64)
!126 = distinct !DILexicalBlock(scope: !124, file: !39, line: 53)
!127 = !DILocalVariable(name: "hb", scope: !128, file: !39, line: 54, type: !42, align: 64)
!128 = distinct !DILexicalBlock(scope: !126, file: !39, line: 54)
!129 = !DILocation(line: 51, scope: !122)
!130 = !DILocation(line: 52, scope: !124)
!131 = !DILocation(line: 51, scope: !117)
!132 = !DILocation(line: 52, scope: !122)
!133 = !DILocation(line: 0, scope: !126)
!134 = !DILocation(line: 0, scope: !128)
!135 = !DILocation(line: 55, scope: !128)
!136 = !DILocation(line: 0, scope: !110, inlinedAt: !137)
!137 = distinct !DILocation(line: 55, scope: !128)
!138 = !DILocation(line: 2301, scope: !110, inlinedAt: !137)
!139 = !DILocation(line: 56, scope: !122)
!140 = !DILocation(line: 56, scope: !117)
!141 = distinct !DISubprogram(name: "demo_static_dispatch", linkageName: "_ZN16llvm_insight_lab20demo_static_dispatch17heb9c55b3ed6042e3E", scope: !15, file: !39, line: 43, type: !118, scopeLine: 43, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !35, templateParams: !8, retainedNodes: !142)
!142 = !{!143, !145}
!143 = !DILocalVariable(name: "a", scope: !144, file: !39, line: 44, type: !14, align: 64)
!144 = distinct !DILexicalBlock(scope: !141, file: !39, line: 44)
!145 = !DILocalVariable(name: "b", scope: !146, file: !39, line: 45, type: !28, align: 64)
!146 = distinct !DILexicalBlock(scope: !144, file: !39, line: 45)
!147 = !DILocation(line: 44, scope: !144)
!148 = !DILocation(line: 45, scope: !146)
!149 = !DILocation(line: 44, scope: !141)
!150 = !DILocation(line: 45, scope: !144)
!151 = !DILocation(line: 46, scope: !146)
!152 = !DILocation(line: 0, scope: !110, inlinedAt: !153)
!153 = distinct !DILocation(line: 46, scope: !146)
!154 = !DILocation(line: 2301, scope: !110, inlinedAt: !153)
!155 = !DILocation(line: 47, scope: !144)
!156 = !DILocation(line: 47, scope: !141)
!157 = !DILocation(line: 0, scope: !100)
!158 = !DILocation(line: 20, scope: !100)
!159 = !DILocation(line: 0, scope: !110, inlinedAt: !160)
!160 = distinct !DILocation(line: 20, scope: !100)
!161 = !DILocation(line: 2301, scope: !110, inlinedAt: !160)
!162 = !DILocation(line: 21, scope: !100)
!163 = !DILocation(line: 0, scope: !67)
!164 = !DILocation(line: 26, scope: !67)
!165 = !DILocation(line: 0, scope: !77, inlinedAt: !166)
!166 = distinct !DILocation(line: 26, scope: !67)
!167 = !DILocation(line: 2377, scope: !77, inlinedAt: !166)
!168 = !DILocation(line: 27, scope: !67)
