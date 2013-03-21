package de.gui.avatar;

import java.io.File;
import java.net.URL;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import de.gui.avatar.RenderThread.RenderingCallback;

public class MainActivity extends Activity 
	implements MediaPlayer.OnCompletionListener, MediaPlayer.OnErrorListener, MediaPlayer.OnPreparedListener 
{

	private static final String TAG = "GuideAvatarTest";
	
	private MediaPlayer mPlayer;
	private AvatarView avatarView;

	private String mediaUriString;
	private URL mediaProxiedURL;
	File cachedMediaFile = null;
	
	private static enum TestAudio {
		SAMPLE_11KHZ_8BIT,
		SAMPLE_16KHZ_16BIT
		;
		
		@SuppressLint("DefaultLocale")
		public File file(Context ctx) {
			return new File(
						String.format(
							"%s/voice/%s.wav", 
							ctx.getFilesDir().getAbsolutePath(), 
							name().toLowerCase().replace("_", "")
						)
			);
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		avatarView = (AvatarView) findViewById(R.id.avatarView);
		
		avatarView.setRenderingCallback(new RenderingCallback() {
			@Override
			public void onRendered() {
				loadEpisode();
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	private void loadEpisode() {
		Log.i(TAG, "loadEpisode");

		try {
			mediaUriString = "http://stg-guide-engine-audio.s3.amazonaws.com/cb32660c-d1ca-421e-ad6d-947399707580.wav"; 
			//TODO: Test remove retrieval instead of local playback to make sure syncing is still good
			
			File testFile = TestAudio.SAMPLE_16KHZ_16BIT.file(getApplicationContext());
			
			Log.w(TAG, "Using HARDCODED file for playback and lipsyncing: " + testFile);
			
			mediaProxiedURL = new URL("file://" + testFile);
			cachedMediaFile = testFile;
			
		} catch (Exception e) {
			Log.e(TAG, "ERROR :" + e.toString(), e);
			return;
		}

		Log.i(TAG, "MediaURL: " + mediaUriString);
		Log.i(TAG, "ProxiedMediaURL: " + mediaProxiedURL);
		Log.i(TAG, "CachedMediaFile: " + cachedMediaFile.getAbsolutePath());

		try {
			if (mPlayer != null && mPlayer.isPlaying()) {
				mPlayer.stop();
				mPlayer.reset();
				mPlayer.release();
				mPlayer = null;
			}
		} catch (Exception e) {
			Log.e(TAG, "Error releasing player", e);
		}

		try {
			mPlayer = new MediaPlayer();
			
			//setting listeners
			mPlayer.setOnPreparedListener(this);
			mPlayer.setOnCompletionListener(this);
			mPlayer.setOnErrorListener(this);
			
			//setting stream
			Log.i(TAG, "Initializing MediaPlayer with URI so it can stream and play it...");
			mPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
			
			Uri mediaUri = Uri.parse(mediaProxiedURL.toString());
			
			if (mediaUri.getScheme().matches("https?")) {
				mPlayer.setDataSource(this, mediaUri);
				
			} else if (mediaUri.getScheme().matches("file")) {
				
				Log.d(TAG, "Playing file from local storage: " + mediaUri.getPath() + " - " + cachedMediaFile.exists());
				mPlayer.setDataSource(mediaUri.getPath());
			}
			
			mPlayer.prepareAsync();

		} catch (Exception e) {
			Log.e(TAG, "Error playing mp3: " + e.getMessage());
		}

		Log.d(TAG, "endLoadEpisode");
	}
	
	@Override
	public void onPrepared(final MediaPlayer mp) {
		Log.i(TAG, "onPrepared");
		mp.setOnBufferingUpdateListener(new OnBufferingUpdateListener() {
			
			@Override
			public void onBufferingUpdate(MediaPlayer mp, int percent) {
				Log.d(TAG, "Buffering: " + percent + "% - Played: " + mp.getCurrentPosition() + ", Duration: " + mp.getDuration());
			}
		});

		new AsyncTask<Void, Void, Void>() {
			@Override
			protected Void doInBackground(Void... params) {

				//Load it only when MediaPlayer has notified us it's ready to play (enough buffer downloaded)
				Log.i(TAG, String.format("Providing Native Lib with local cache file reference: %s - Exists: %s - Size: %d", cachedMediaFile, cachedMediaFile.exists(), cachedMediaFile.length()));
				avatarView.lipSyncLoad(cachedMediaFile.getAbsolutePath()); 
				avatarView.lipSyncStart();

				mp.start();
				
				do {
					Log.d(TAG, "Waiting to load MediaPlayer");
				} while (!mp.isPlaying());

				Log.d(TAG, "Media Prepared");

				return null;
			}
		}.execute();
	}
	
	public void onCompletion(MediaPlayer mp) {
		Log.i(TAG, "audio finished");
		
		mp.stop();
		mp.reset();
		mp.release();
		mp = null;

		//replay
		loadEpisode();
	}

	@Override
	public boolean onError(MediaPlayer mp, int what, int extra) {
		
		Log.e(TAG, String.format("Error playing audio: WHAT=%s, EXTRA=%s", what, extra));
		
		return false;
	}
	
}
