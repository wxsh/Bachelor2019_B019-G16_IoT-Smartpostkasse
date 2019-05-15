package no.hiof.denniss.iot_smartpostkasse;


import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.google.android.gms.tasks.OnCompleteListener;
import com.google.android.gms.tasks.Task;
import com.google.firebase.iid.FirebaseInstanceId;
import com.google.firebase.iid.InstanceIdResult;


import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.text.DateFormat;
import java.util.Date;

import no.hiof.denniss.iot_smartpostkasse.Service.MyFirebaseMessagingService;


public class MainActivity extends AppCompatActivity {

    //Her lagres brukernavn
    SharedPreferences sharedPreferences;

    //her er alle feltene i layout.
    TextView dataReceived;
    ProgressBar mailboxBatteryBar;
    Button clearButton;
    TextView btryPrcnt;
    TextView username;

    String TAG = "mainActivity";

    String filename = "myfile";
    String filename2 = "batteryFile";

    int battery;
    int status;
    long dato;


    //En funksjon som henter opp tidligere data.
    void loadFile(){
        String yourFilePath = getFilesDir() + "/" + filename;
        String yourFilePath2 = getFilesDir() + "/" + filename2;

        File file = new File( yourFilePath );
        File file2 = new File( yourFilePath2 );

        StringBuilder text = new StringBuilder();

        try {
            BufferedReader br = new BufferedReader(new FileReader(file));
            String line;

            while ((line = br.readLine()) != null) {
                text.append(line);
                text.append('\n');
            }
            br.close();
        }
        catch (IOException e) {
            //You'll need to add proper error handling here
        }
        dataReceived.setText(text.toString());

        text = new StringBuilder();
        try {
            BufferedReader br = new BufferedReader(new FileReader(file2));
            String line;

            while ((line = br.readLine()) != null) {
                text.append(line);
            }
            br.close();
            if(!text.toString().equals("")){
                battery = Integer.parseInt(text.toString());
                mailboxBatteryBar.setProgress(battery);
                btryPrcnt.setText("" + battery + "%");
            }
        }
        catch (IOException e) {
            //You'll need to add proper error handling here
        }


    }

    //En funskjson som lagrer data
    void saveFile(){
        String fileContents = dataReceived.getText().toString();
        String fileContents2 = "" + battery;
        FileOutputStream outputStream;

        try {
            outputStream = openFileOutput(filename, MODE_PRIVATE);
            outputStream.write(fileContents.getBytes());

            outputStream = openFileOutput(filename2, MODE_PRIVATE);
            outputStream.write(fileContents2.getBytes());
            outputStream.close();
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    //En funksjon som tar en string array med strings fra firebase notifikasjoner of henter ut vår data
    void getInfo(String[] info){

        double volt = Integer.valueOf(info[0].split(":")[1]);
        volt = (volt/1023.0)*6.6;
        int voltPercent = (int)(100/1.2*(volt-3));

        status  = Integer.valueOf(info[1].split(":")[1]);
        dato  = Long.valueOf(info[2].split(":")[1]);
        String statusMessage = "";

        Date date = new Date(dato);

        //Bestemmer hvilke melding som kommer  basert på status fra postkassen
        switch(status) {
            case 1:
                statusMessage = "Du har fått ny post!";
                break;
            case 2:
                statusMessage = "Posten har blitt tatt ut av postkassen";
                break;
            case 3:
                statusMessage = "Postkassen ble åpnet, men ingen ny post";
                break;
            default:
                statusMessage = "Ikke godkjent statuskode.";
                break;

        }

        if(voltPercent >= 100) {
            voltPercent = 100;
        }

        if(voltPercent <= 0) {
            voltPercent = 0;
        }

        //Viser data på skjermen
        dataReceived.setText(DateFormat.getDateTimeInstance().format(date) + "\n" + statusMessage + "\n\n" + dataReceived.getText());
        mailboxBatteryBar.setProgress(voltPercent);
        btryPrcnt.setText(voltPercent + "%");
        battery = voltPercent;

        saveFile();

    }

    //Sjekker om det er data i notfikasjonen. Er det det sender den en splittet streng array vidre.
    void getNotificationData(){
        Bundle extra = getIntent().getExtras();
        if (extra != null) {

            try {
                Log.w("info", extra.getString("information"));

                //DONE: store alle info in the right fields

                String[] info = extra.getString("information").split(",");
                getInfo(info);

            }catch (Exception e){
                Log.w("info", e.fillInStackTrace());
            }
        }
    }


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        sharedPreferences = PreferenceManager
                .getDefaultSharedPreferences(this);

        dataReceived = (TextView) findViewById(R.id.dataReceived);
        mailboxBatteryBar = (ProgressBar) findViewById(R.id.progressBar);
        clearButton = (Button) findViewById(R.id.clearBtn);
        btryPrcnt = (TextView) findViewById(R.id.pstBtryPrcnt);
        username = (TextView) findViewById(R.id.username);

        clearButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Knappen som sletter alle tidligere meldinger.
                dataReceived.setText("");
                saveFile();
            }
        });

        //Dette er funksjonen som gjør at "Skriv inn brukernavn" delen kjører hvis et brukernavn ikke er lagt til.
        if (!sharedPreferences.contains("username")){
            Intent myIntent = new Intent(MainActivity.this, GetUsername.class);
            MainActivity.this.startActivity(myIntent);
        }
        else
            username.setText("Brukernavn: " + sharedPreferences.getString("username", "") );

        //Denne delen lager og sender den unike iden til firebase som gjør at man kan sende notfikasjoner.
        FirebaseInstanceId.getInstance().getInstanceId()
                .addOnCompleteListener(new OnCompleteListener<InstanceIdResult>() {
                    @Override
                    public void onComplete(@NonNull Task<InstanceIdResult> task) {
                        if (!task.isSuccessful()) {
                            Log.w(TAG, "getInstanceId failed", task.getException());
                            return;
                        }

                        // Get new Instance ID token
                        String token = task.getResult().getToken();

                        MyFirebaseMessagingService.sendRegistrationToServer(token, sharedPreferences.getString("username", "testuser"));

                        // Log and toast
                        String msg = getString(R.string.msg_token_fmt, token);
                        Log.d(TAG, msg);
                        //Toast.makeText(MainActivity.this, msg, Toast.LENGTH_SHORT).show();

                    }
                });

        loadFile();
        //get notification data
        getNotificationData();

        saveFile();

    }

    @Override
    protected void onStart() {
        super.onStart();
        //Denne delen starter broadcasmanager som gjør man kan hente data fra en service
        LocalBroadcastManager.getInstance(this).registerReceiver((mMessageReceiver),
                new IntentFilter("MyData")
        );
    }

    @Override
    protected void onStop() {
        super.onStop();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mMessageReceiver);
    }

    @Override
    protected void onResume() {
        super.onResume();


    }

    @Override
    protected void onPause() {
        super.onPause();

    }


    private BroadcastReceiver mMessageReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {


            //henter info hvis appen kjører i forgrunnen.
            try {
                String[] info = (intent.getExtras().getString("info")).split(",");
                getInfo(info);

            }catch (Exception e){
                Log.w("info", e.fillInStackTrace());
            }


        }
    };

}
