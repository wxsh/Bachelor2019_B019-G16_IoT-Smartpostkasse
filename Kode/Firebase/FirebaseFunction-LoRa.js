// The Cloud Functions for Firebase SDK to create Cloud Functions and setup triggers.
const functions = require('firebase-functions');
// The Firebase Admin SDK to access the Firebase Realtime Database.
const admin = require('firebase-admin');
admin.initializeApp();

// Take the text parameter passed to this HTTP endpoint and insert it into the
// Realtime Database under the path /payload/:pushId/original
exports.postReceived2 = functions.https.onRequest((req, res) => {
    if (req.method !== 'POST') {
	return res.status(403).send('Forbidden!');
    }
    var registrationToken = '';
    // Grab the body parameter.
    const info = req.body;
    var user = req.query.username;
    // Push the new message into the Realtime Database using the Firebase Admin SDK.
    const date = new Date();
    var volt = parseInt(info.split(",")[0].split(":")[1]);
    var status = parseInt(info.split(",")[1].split(":")[1]);
    var infoString = "volt:" + volt + ",status:" + status + ",dato:" + date.getTime();
    admin.database().ref('users/' + user + '/messages').push({ info: infoString});
    admin.database().ref('users/' + user + "/hasNewMessage" ).set({ bool: true, lastMessage: infoString});
    var db = admin.database();
    var reff = db.ref('users/' + user + '/ID');
    var notificationTitle = "";
    var notificationBody = "";

    switch(status) {
      case 1:
        notificationTitle = "Post ankommet!";
        notificationBody = "Det har blitt levert post i postkassen";
      	break;
      case 2:
        notificationTitle = "Post fjernet";
        notificationBody = "Posten har blitt hentet";
        break;
      case 3:
        notificationTitle = "Postkasse åpnet";
        notificationBody = "Postkassen har åpnet, men ingen ny post";
        break;
      default:
        break;
    }
  
  
    return reff.once("value", function (snapshot) {
        registrationToken = snapshot.val();
        console.log(snapshot.val());

        const payload = {
            notification: {
                title: notificationTitle,
                body: notificationBody,
                icon: 'default',
            },
            data: {
                information: infoString
            }
        };
        // You must return a Promise when performing asynchronous tasks inside a Functions such as
        // writing to the Firebase Realtime Database.
        // Setting an "uppercase" sibling in the Realtime Database returns a Promise.
        res.end();
        return admin.messaging().sendToDevice(registrationToken, payload)
    });
});
