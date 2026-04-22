// WikiRadius v2.0.0 - High Contrast Edition (PebbleKit JS)

function sendStatus(message) {
  Pebble.sendAppMessage({'MESSAGE_KEY_STATUS': message});
}

function fetchWikipediaData(lat, lon) {
  sendStatus("Wikipedia Suche...");
  var url = 'https://en.wikipedia.org/w/api.php?action=query&list=geosearch&gscoord=' + lat + '|' + lon + '&gsradius=5000&gslimit=1&format=json';
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.readyState === 4 && req.status === 200) {
      var response = JSON.parse(req.responseText);
      if (response.query.geosearch.length > 0) {
        var pageId = response.query.geosearch[0].pageid;
        var title = response.query.geosearch[0].title;
        var dist = response.query.geosearch[0].dist;
        var bearing = Math.floor(Math.random() * 360); // Simuliertes Bearing für Navigation
        
        fetchWikipediaSnippet(pageId, title, dist, bearing);
      } else {
        sendStatus("Nichts in 5km gefunden.");
      }
    } else {
      sendStatus("Wiki API Fehler.");
    }
  };
  req.onerror = function() { sendStatus("Netzwerkfehler."); };
  req.send(null);
}

function fetchWikipediaSnippet(pageId, title, dist, bearing) {
  sendStatus("Lade Artikel...");
  var url = 'https://en.wikipedia.org/w/api.php?action=query&prop=extracts&exchars=180&exintro&explaintext&pageids=' + pageId + '&format=json';
  
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function() {
    if (req.readyState === 4 && req.status === 200) {
      var response = JSON.parse(req.responseText);
      var snippet = response.query.pages[pageId].extract;
      
      var dict = {
        'MESSAGE_KEY_TITLE': title,
        'MESSAGE_KEY_SNIPPET': snippet,
        'MESSAGE_KEY_DISTANCE': Math.round(dist),
        'MESSAGE_KEY_BEARING': bearing
      };
      
      Pebble.sendAppMessage(dict, function() {
        console.log('Daten erfolgreich gesendet!');
      }, function(e) {
        console.log('Fehler beim Senden der Daten.');
      });
    }
  };
  req.send(null);
}

function getGPS() {
  sendStatus("Warte auf GPS-Fix...");
  navigator.geolocation.getCurrentPosition(
    function(pos) {
      fetchWikipediaData(pos.coords.latitude, pos.coords.longitude);
    },
    function(err) {
      sendStatus("GPS fehlgeschlagen.");
    },
    { timeout: 15000, maximumAge: 0, enableHighAccuracy: true }
  );
}

// Hört auf das "Go!"-Signal vom C-Code
Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.MESSAGE_KEY_READY) {
    getGPS();
  }
});

Pebble.addEventListener('ready', function(e) {
  console.log('JS bereit. Warte auf Signal der Uhr.');
});
