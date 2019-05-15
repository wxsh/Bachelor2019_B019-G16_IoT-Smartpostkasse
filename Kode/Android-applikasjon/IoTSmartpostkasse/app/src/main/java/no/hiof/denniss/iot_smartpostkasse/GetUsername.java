package no.hiof.denniss.iot_smartpostkasse;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.text.InputType;
import android.view.View;
import android.widget.EditText;

//En activity som kun skal hente ut brukernavn hvis det ikke er satt fra før
public class GetUsername extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final SharedPreferences sharedPreferences = PreferenceManager
                .getDefaultSharedPreferences(this);


        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Skriv inn tildelt brukernavn:");

        final EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL);
        builder.setView(input);

        //Setter opp knappene
        builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                sharedPreferences.edit().putString("username", input.getText().toString()).apply();
                Intent myIntent = new Intent(GetUsername.this, MainActivity.class);
                GetUsername.this.startActivity(myIntent);
            }
        });

        builder.show();

    }

}
