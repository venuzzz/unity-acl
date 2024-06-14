#pragma once
#define UNITY_ANDROID
#include <string>
#include <vector>
#include "Unity/includes/IUnityInterface.h"
#include "acl/decompression/decompress.h"

using default_decompression_type = acl::decompression_context<acl::decompression_settings>;

struct my_track_writer : public acl::track_writer
{
	my_track_writer(void* _qvvf, char* _flags): qvvf((rtm::qvvf*)_qvvf), flags(_flags)
	{
	}
	
	void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
	{
		qvvf[track_index].rotation = rotation;
		flags[track_index] |= 1;
	}

	//////////////////////////////////////////////////////////////////////////
	// Called by the decoder to write out a translation value for a specified bone index.
	void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
	{
		qvvf[track_index].translation = translation;
		flags[track_index] |= 2;
	}

	//////////////////////////////////////////////////////////////////////////
	// Called by the decoder to write out a scale value for a specified bone index.
	void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
	{
		qvvf[track_index].scale = scale;
		flags[track_index] |= 4;
	}

	// skip track with default value
	static constexpr acl::default_sub_track_mode get_default_rotation_mode() { return acl::default_sub_track_mode::skipped; }
	static constexpr acl::default_sub_track_mode get_default_translation_mode() { return acl::default_sub_track_mode::skipped; }
	static constexpr acl::default_sub_track_mode get_default_scale_mode() { return acl::default_sub_track_mode::skipped; }
	
	rtm::qvvf* qvvf;
	char* flags;
};

// Quaternation.Angle
RTM_DISABLE_SECURITY_COOKIE_CHECK RTM_FORCE_INLINE float RTM_SIMD_CALL quat_angle_distance(rtm::quatf_arg0 lhs, rtm::quatf_arg0 rhs, float threshold = 0.00001F) RTM_NO_EXCEPT
{
	rtm::vector4f lhs_vector = rtm::quat_to_vector(lhs);
	rtm::vector4f rhs_vector = rtm::quat_to_vector(rhs);
	float dot = rtm::vector_dot(lhs_vector, rhs_vector);
    return (dot > 1 - threshold) ? 0.0f : rtm::scalar_acos(rtm::scalar_min(rtm::scalar_abs(dot), 1.0F)) * 2.0F;
}

struct my_track_writer_ex : my_track_writer
{
	my_track_writer_ex(void* _qvvf, char* _flags, float _pos_threshold, float _rot_threshold, float _scale_threshold): my_track_writer(_qvvf, _flags), pos_threshold(_pos_threshold), rot_threshold(_rot_threshold), scale_threshold(_scale_threshold)
	{
	}
	
	void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation)
	{
		//if (quat_angle_distance(rotation, qvvf[track_index].rotation) >= rot_threshold)
		if (!rtm::vector_all_near_equal(rotation, qvvf[track_index].rotation, rot_threshold))
		{
			qvvf[track_index].rotation = rotation;
			flags[track_index] |= 1;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Called by the decoder to write out a translation value for a specified bone index.
	void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation)
	{
		//if (rtm::vector_distance3(translation, qvvf[track_index].translation) >= pos_threshold)
		if (!rtm::vector_all_near_equal3(translation, qvvf[track_index].translation, pos_threshold))
		{
			qvvf[track_index].translation = translation;
			flags[track_index] |= 2;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Called by the decoder to write out a scale value for a specified bone index.
	void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale)
	{
		//if (rtm::vector_distance3(scale, qvvf[track_index].scale) >= scale_threshold)
		if (!rtm::vector_all_near_equal3(scale, qvvf[track_index].scale, scale_threshold))
		{
			qvvf[track_index].scale = scale;
			flags[track_index] |= 4;
		}
	}
	
	float pos_threshold;
	float rot_threshold;
	float scale_threshold;
};

//use unity macro to check android.
void* x_aligned_malloc(size_t sz, size_t align)
{
	void *result = nullptr;
#ifdef _MSC_VER 
	result = _aligned_malloc(sz, align);
#elif __APPLE__
	if (posix_memalign(&result, align, sz)) result = nullptr;
#elif UNITY_ANDROID
	//__linux__
	result = memalign(align, sz);
#endif
	return result;
}

extern "C"    UNITY_INTERFACE_EXPORT  bool IsCompressedTracksValid (void* buffer)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	return (tracks != nullptr && tracks->is_valid(false).empty());
}

extern "C"  UNITY_INTERFACE_EXPORT   int GetNumTracks (void* buffer)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	if (tracks != nullptr)
		return tracks->get_num_tracks();
	return 0;
}

extern "C"   UNITY_INTERFACE_EXPORT float GetDuration (void* buffer)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	if (tracks != nullptr)
		return tracks->get_duration();
	return 0;
}

extern "C" UNITY_INTERFACE_EXPORT  const char*  GetTrackName (void* buffer, int track_index)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	if (tracks != nullptr)
		return tracks->get_track_name(track_index);
	return nullptr;
}

extern "C"   UNITY_INTERFACE_EXPORT int GetParentTrackIndex (void* buffer, int track_index)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	if (tracks != nullptr)
		return tracks->get_parent_track_index(track_index);
	return acl::k_invalid_track_index;
}

extern "C"   UNITY_INTERFACE_EXPORT void* PrepareDecompressContext (void* buffer)
{
	const auto& tracks = acl::make_compressed_tracks(buffer, nullptr);
	if (!tracks)
	{
		return nullptr;
	}

	const auto context = new (x_aligned_malloc(sizeof(default_decompression_type), 64))default_decompression_type();
	if (!context->initialize(*tracks))
	{
		((default_decompression_type*)context)->~decompression_context();
		_aligned_free(context);
		return nullptr;
	}
	
	return context;
}

extern "C"  UNITY_INTERFACE_EXPORT void  ReleaseDecompressContext (void* context)
{
	if (context)
	{
		((default_decompression_type*)context)->~decompression_context();
		_aligned_free(context);
	}
}

extern "C"   UNITY_INTERFACE_EXPORT void SeekInContext (void* context, float sample_time, int rounding_policy)
{
	if (context)
	{
		((default_decompression_type*)context)->seek(sample_time, (acl::sample_rounding_policy)rounding_policy);
	}
}

extern "C"   UNITY_INTERFACE_EXPORT void DecompressTracks (void* context, void* result, char* flags)
{
	if (context)
	{
		int num_tracks = ((default_decompression_type*)context)->get_compressed_tracks()->get_num_tracks();
		memset(result, 0, num_tracks * 12 * sizeof(float));
		memset(flags, 0, num_tracks * sizeof(char));
		my_track_writer writer(result, flags);
		((default_decompression_type*)context)->decompress_tracks(writer);
	}
}

extern "C"   UNITY_INTERFACE_EXPORT void DecompressTracksEx (void* context, void* result, char* flags, float pos_threshold, float rot_threshold, float scale_threshold)
{
	if (context)
	{
		int num_tracks = ((default_decompression_type*)context)->get_compressed_tracks()->get_num_tracks();
		memset(flags, 0, num_tracks * sizeof(char));
		my_track_writer_ex writer(result, flags, pos_threshold, rot_threshold, scale_threshold);
		((default_decompression_type*)context)->decompress_tracks(writer);
	}
}


