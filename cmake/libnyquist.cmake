
#-------------------------------------------------------------------------------

project(libopus)

set(      third_opus
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/bands.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/celt.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/celt_decoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/celt_encoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/celt_lpc.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/cwrs.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/entcode.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/entdec.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/entenc.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/kiss_fft.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/laplace.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/mathops.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/mdct.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/modes.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/pitch.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/quant_bands.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/rate.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt/vq.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus_decoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus_encoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus_multistream.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus_multistream_decoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/src/opus_multistream_encoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/info.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/internal.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/opusfile.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/stream.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/bwexpander.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/bwexpander_32.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/CNG.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/code_signs.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/dec_API.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_core.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_frame.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_indices.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_parameters.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_pitch.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decode_pulses.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/decoder_set_fs.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/gain_quant.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/init_decoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/init_encoder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/lin2log.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/log2lin.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/LPC_analysis_filter.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/LPC_inv_pred_gain.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF_decode.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF_encode.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF_stabilize.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF_unpack.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF_VQ_weights_laroia.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/NLSF2A.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/PLC.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/pitch_est_tables.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler_private_AR2.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler_private_down_FIR.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler_private_IIR_FIR.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler_private_up2_HQ.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/resampler_rom.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/shell_coder.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/sort.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/stereo_decode_pred.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/stereo_encode_pred.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/stereo_MS_to_LR.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/sum_sqr_shift.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/table_LSF_cos.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_gain.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_LTP.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_NLSF_CB_NB_MB.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_NLSF_CB_WB.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_other.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_pitch_lag.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/tables_pulses_per_block.c"
)

add_library(libopus STATIC 
    ${third_opus})

_set_Cxx17(libopus)
_set_compile_options(libopus)
_disable_warning(4244)
_disable_warning(4018)

target_include_directories(libopus PRIVATE
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libogg/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/float)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(libnyquist PROPERTIES PREFIX "../")
    set_target_properties(libopus PROPERTIES IMPORT_PREFIX "../")
endif()

target_compile_definitions(libopus PRIVATE OPUS_BUILD)
target_compile_definitions(libopus PRIVATE USE_ALLOCA)

set_target_properties(libopus
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(libopus PROPERTIES OUTPUT_NAME_DEBUG libopus_d)

install (TARGETS libopus
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install (TARGETS libopus DESTINATION lib)

# folders

source_group(src\\ FILES ${third_opus})


#-------------------------------------------------------------------------------

project(libnyquist)

file(GLOB third_nyquist_h   "${LABSOUND_ROOT}/third_party/libnyquist/include/libnyquist/*")
set(      third_nyquist_src
    "${LABSOUND_ROOT}/third_party/libnyquist/src/AudioDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/CafDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/Common.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/FlacDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/FlacDependencies.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/MusepackDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/OpusDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/RiffUtils.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/VorbisDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/VorbisDependencies.c"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/WavDecoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/WavEncoder.cpp"
    "${LABSOUND_ROOT}/third_party/libnyquist/src/WavPackDecoder.cpp"
)
file(GLOB third_musepack_dec "${LABSOUND_ROOT}/third_party/libnyquist/third_party/musepack/libmpcdec/*")
file(GLOB third_musepack_enc "${LABSOUND_ROOT}/third_party/libnyquist/third_party/musepack/libmpcenc/*")
file(GLOB third_wavpack      "${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/src/*")

add_library(libnyquist STATIC 
    ${third_nyquist_h} ${third_nyquist_src}
    ${third_musepack_dec} ${third_musepack_enc}
    ${third_wavpack})

_set_Cxx17(libnyquist)
_set_compile_options(libnyquist)
_disable_warning(4244)
_disable_warning(4018)

target_include_directories(libnyquist PRIVATE
    ${LABSOUND_ROOT}/third_party/libnyquist/include
    ${LABSOUND_ROOT}/third_party/libnyquist/include/libnyquist
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/flac/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libogg/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/libvorbis/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/musepack/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/celt
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/libopus/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/opusfile/src/include
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/opus/silk/float
    ${LABSOUND_ROOT}/third_party/libnyquist/third_party/wavpack/include
    ${LABSOUND_ROOT}/third_party/libnyquist/src)

if (MSVC_IDE)
    # hack to get around the "Debug" and "Release" directories cmake tries to add on Windows
    #set_target_properties(libnyquist PROPERTIES PREFIX "../")
    set_target_properties(libnyquist PROPERTIES IMPORT_PREFIX "../")
endif()

set_target_properties(libnyquist PROPERTIES OUTPUT_NAME_DEBUG libnyquist_d)

set_target_properties(libnyquist
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(libnyquist libopus)

install (TARGETS libnyquist
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install (TARGETS libnyquist DESTINATION lib)

# folders

source_group(include\\libnyquist FILES ${third_nyquist_h})
source_group(src FILES ${third_nyquist_src})
source_group(third_party\\musepack\\ FILES ${third_musepack_enc} ${third_musepack_dec})
source_group(third_party\\wavpack\\ FILES ${third_wavpack})
