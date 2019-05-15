package no.hiof.denniss.iot_smartpostkasse.Service;

import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.widget.TextView;

import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.messaging.FirebaseMessaging;
import com.google.firebase.messaging.FirebaseMessagingService;
import com.google.firebase.messaging.RemoteMessage;

import java.text.DateFormat;
import java.util.Date;

import no.hiof.denniss.iot_smartpostkasse.MainActivity;
import no.hiof.denniss.iot_smartpostkasse.R;

public class MyFirebaseMessagingService extends FirebaseMessagingService {

    //Den lokale broadcastmanager som gjør at den kan sende data til en activity.
    private LocalBroadcastManager broadcaster;

    public MyFirebaseMessagingService() {
    }

    String TAG = "firebaseApp";

    SharedPreferences sharedPreferences;

    @Override
    public void onCreate() {
        super.onCreate();

        sharedPreferences = PreferenceManager
                .getDefaultSharedPreferences(this);

        broadcaster = LocalBroadcastManager.getInstance(this);
    }

    //Funksjonen som kjøres når applikasjonen får en notifikasjon.
    @Override
    public void onMessageReceived(RemoteMessage remoteMessage) {
        Log.d(TAG, "From: " + remoteMessage.getFrom());

        //Sjekker om meldingen innholder data
        if (remoteMessage.getData().size() > 0) {

            Log.d(TAG, "Message data payload: " + remoteMessage.getData());

            //henter ut informasjonen fra notifikasjonen og lagrer den som intent som kan bli hentet av en aktivitet fra broadcasmanager.
            Intent intent = new Intent("MyData");
            intent.putExtra("info", remoteMessage.getData().get("information"));
            broadcaster.sendBroadcast(intent);

        }

        // Sjekker om meldingen inneholder en notifikasjon.
        if (remoteMessage.getNotification() != null) {
            Log.d(TAG, "Message Notification Body: " + remoteMessage.getNotification().getBody());
        }
    }

    /**
     * Called if InstanceID token is updated. This may occur if the security of
     * the previous token had been compromised. Note that this is called when the InstanceID token
     * is initially generated so this is where you would retrieve the token.
     */

    //Kjøres hvis det bli generet en ny token
    @Override
    public void onNewToken(String token) {
        Log.d(TAG, "Refreshed token: " + token);
        String username = sharedPreferences.getString("username", "testuser");

        FirebaseMessaging.getInstance();
        sendRegistrationToServer(token, username);
    }
    //Sender de nye token til rikig sted i firebase
    public static void sendRegistrationToServer(String token, String username) {

        final FirebaseDatabase database = FirebaseDatabase.getInstance();
        DatabaseReference ref = database.getReference("users/" + username+ "/ID");
        // then store your token ID
        ref.setValue(token);
    }
}
