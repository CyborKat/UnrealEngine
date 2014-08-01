// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Enumerates directions for seeking in media.
 */
enum class EMediaSeekDirection
{
	/** Seek backwards from current position. */
	Backward,

	/** Seek from the beginning of the media. */
	Beginning,

	/** Seek from the end of the media. */
	End,

	/** Seek forward from current position. */
	Forward
};


/** Type definition for shared pointers to instances of IMediaPlayer. */
typedef TSharedPtr<class IMediaPlayer> IMediaPlayerPtr;

/** Type definition for shared references to instances of IMediaPlayer. */
typedef TSharedRef<class IMediaPlayer> IMediaPlayerRef;


/**
 * Interface for media players.
 *
 * @see IMediaTrack
 */
class IMediaPlayer
{
public:

	/**
	 * Closes a previously opened media.
	 *
	 * If no media has been opened, this function has no effect.
	 *
	 * @see IsReady, Open
	 */
	virtual void Close( ) = 0;

	/**
	 * Gets the media's duration.
	 *
	 * @return A time span representing the duration.
	 * @see GetTime, Seek
	 */
	virtual FTimespan GetDuration( ) const = 0;

	/**
	 * Gets the last error that occurred during media loading or playback.
	 *
	 * @return The error string, or an empty string if no error occurred.
	 */
//	virtual FString GetLastError( ) const = 0;

	/**
	 * Gets the nominal playback rate, i.e. 1.0 for real time.
	 *
	 * @return Playback rate.
	 * @see Play
	 */
	virtual float GetRate( ) const = 0;

	/**
	 * Gets the player's current playback time.
	 *
	 * @return Playback time.
	 * @see GetDuration, Seek
	 */
	virtual FTimespan GetTime( ) const = 0;

	/**
	 * Gets access to the media's audio, video and other tracks.
	 *
	 * @return Media tracks interface.
	 */
	virtual const TArray<IMediaTrackRef>& GetTracks( ) const = 0;

	/**
	 * Gets the URL of the currently loaded media.
	 *
	 * @return Media URL.
	 */
	virtual FString GetUrl( ) const = 0;

	/**
	 * Checks whether media playback is currently paused.
	 *
	 * @return true if playback is paused, false otherwise.
	 * @see IsPlaying, IsStopped, Pause, Play, Stop
	 */
	virtual bool IsPaused( ) const = 0;

	/**
	 * Checks whether media playback has currently in progress.
	 *
	 * @return true if playback is in progress, false otherwise.
	 * @see IsPaused, IsStopped, Pause, Play, Stop
	 */
	virtual bool IsPlaying( ) const = 0;

	/**
	 * Checks whether this player is ready for playback.
	 *
	 * A media player is considered ready if some media has been opened
	 * successfully using the Open method and no error occurred during
	 * loading or playback.
	 *
	 * @return true if ready, false otherwise.
	 * @see Close, Open
	 */
	virtual bool IsReady( ) const = 0;

	/**
	 * Opens a media from a file on disk.
	 *
	 * @param Url The URL of the media to open (file name or web address).
	 * @return true if the media was opened successfully, false otherwise.
	 * @see Close, IsReady
	 */
	virtual bool Open( const FString& Url ) = 0;

	/**
	 * Opens a media from a buffer.
	 *
	 * @param Buffer The buffer holding the media data.
	 * @param OriginalUrl The original URL of the media that was loaded into the buffer.
	 * @return true if the media was opened, false otherwise.
	 * @see Close, IsReady
	 */
	virtual bool Open( const TSharedRef<TArray<uint8>>& Buffer, const FString& OriginalUrl ) = 0;

	/**
	 * Starts media playback at the specified rate.
	 *
	 * A playback rate of 1.0 will play the media normally at real-time.
	 * A rate of 0.0 corresponds to pausing playback. A negative rate, if
	 * supported, plays the media in reverse, and a rate larger than 1.0
	 * fast forwards playback.
	 *
	 * @param Rate The playback rate to use.
	 * @return true if the media will be played, false otherwise.
	 * @see GetRate, Pause, Stop
	 */
	virtual bool Play( float Rate ) = 0;

	/**
	 * Changes the media's playback time.
	 *
	 * @param Time The playback time to set.
	 * @return true on success, false otherwise.
	 * @see GetDuration, GetTime
	 */
	virtual bool Seek( const FTimespan& Time ) = 0;

	/**
	 * Checks whether the specified playback rate is supported.
	 *
	 * @param Rate The rate to check (can be negative for reverse play).
	 * @param Unthinned Whether no frames should be dropped at the given rate.
	 * @return true if the rate is supported, false otherwise.
	 * @see SupportsScrubbing, SupportsSeeking
	 */
	virtual bool SupportsRate( float Rate, bool Unthinned ) const = 0;

	/**
	 * Checks whether scrubbing is supported.
	 *
	 * Scrubbing is the ability to decode video frames while seeking in a media item at a playback rate of 0.0.
	 *
	 * @return true if scrubbing is supported, false otherwise.
	 * @see SupportsRate, SupportsSeeking
	 */
	virtual bool SupportsScrubbing( ) const = 0;

	/**
	 * Checks whether the currently loaded media can jump to certain times.
	 *
	 * @return true if seeking is supported, false otherwise.
	 * @see SupportsRate, SupportsScrubbing
	 */
	virtual bool SupportsSeeking( ) const = 0;

public:

	/** Gets an event delegate that is invoked when media has been closed. */
	DECLARE_EVENT_OneParam(IMediaPlayer, FOnMediaClosing, FString /*ClosedUrl*/)
	virtual FOnMediaClosing& OnClosing( ) = 0;

	/** Gets an event delegate that is invoked when media has been opened. */
	DECLARE_EVENT_OneParam(IMediaPlayer, FOnMediaOpened, FString /*OpenedUrl*/)
	virtual FOnMediaOpened& OnOpened( ) = 0;

public:

	/**
	 * Attempts to get the media track with the specified index and type.
	 *
	 * @param TrackIndex The index of the track to get.
	 * @param TrackType The expected type of the track, i.e. audio or video.
	 * @return The track if it exists, nullptr otherwise.
	 */
	IMediaTrackPtr GetTrackSafe( int32 TrackIndex, EMediaTrackTypes TrackType )
	{
		if (GetTracks().IsValidIndex(TrackIndex))
		{
			IMediaTrackPtr Track = GetTracks()[TrackIndex];

			if (Track->GetType() == TrackType)
			{
				return Track;
			}
		}

		return nullptr;
	}

	/**
	 * Pauses media playback.
	 *
	 * @return true if the media is being paused, false otherwise.
	 * @see Play, Stop
	 */
	FORCEINLINE bool Pause( )
	{
		return Play(0.0f);
	}

	/**
	 * Starts media playback at the default rate of 1.0.
	 *
	 * This method is equivalent to setting the playback rate to 1.0.
	 *
	 * @return true if playback is starting, false otherwise.
	 * @see Pause, Stop
	 */
	FORCEINLINE bool Play( )
	{
		return Play(1.0f);
	}

	/**
	 * Changes the playback time of the media by a relative offset in the given direction.
	 *
	 * @param TimeOffset The offset to apply to the time.
	 * @param Direction The direction to seek in.
	 * @return true on success, false otherwise.
	 * @see GetDuration, GetTime
	 */
	bool Seek( const FTimespan& TimeOffset, EMediaSeekDirection Direction )
	{
		FTimespan SeekTime;

		switch (Direction)
		{
		case EMediaSeekDirection::Backward: SeekTime = GetTime() - TimeOffset; break;
		case EMediaSeekDirection::Beginning: SeekTime = TimeOffset; break;
		case EMediaSeekDirection::End: SeekTime = GetDuration() - TimeOffset; break;
		case EMediaSeekDirection::Forward: SeekTime = GetTime() + TimeOffset; break;
		}

		return Seek(SeekTime);
	}

public:

	/** Virtual destructor. */
	virtual ~IMediaPlayer( ) { }
};
