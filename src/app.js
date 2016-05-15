var API_OPTIONS = "perspective=member&restrict_kind=productivity&resolution_time=day&format=json";

Pebble.addEventListener('showConfiguration', function (e) {
  Pebble.openURL('http://ntikhonov.com/save-time-config/index.html');
});

Pebble.addEventListener("webviewclosed", function (e) {
  var rt = typeof e.response,
      settings = (rt === "undefined" ? {} : JSON.parse(decodeURIComponent(e.response)));
  if (Object.keys(settings).length > 0) {
    var key = settings.api_key.trim();
    if (key !== "") { // A key was actually entered
      localStorage.setItem("api_key", key);
      getProductivity();
    }
  }
});

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function getProductivity() {
  // Construct URL
  var api_key = localStorage.getItem("api_key");
  if (!api_key) {
    sendStringUpdate("no","key");
  } else {
    var url = 'https://www.rescuetime.com/anapi/data?key=' + api_key + '&' + API_OPTIONS;

    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET', 
      function(responseText) {
        // responseText contains a JSON object with weather info
        var response = JSON.parse(responseText);
        var data = calcStats(response);
        if (data) {
          sendUpdate(data.score, data.total);
        } else {
          sendUpdate(0,0);
        }
      }      
    );
  }
}

function calcStats(json) {
  var times = [0,0,0,0,0];

  var i = 0;
  for (i = 0; i < json.rows.length; i++) {
    var row = json.rows[i];
    var time = row[1];
    var productivity = row[2];

    times[productivity + 2] += time;
  }

  var scores = [0,25,50,75,100];

  var total = times.reduce(function(a,b) {
    return a + b;
  }, 0);
  
  if (total === 0) return undefined;
  
  var score = 0;

  for (i = 0; i < scores.length; i++) {
    score += (times[i] / total) * scores[i];
  }

  total = Math.floor(total / 3600);
  score = Math.floor(score);
  
  return {
    total: total,
    score: score
  };
}

function sendUpdate(productivity, time) {
  sendStringUpdate(productivity + "%", time + ((time === 1) ? "hr" : "hrs"));
}

function sendStringUpdate(line1, line2) {
  var dictionary = {
    "KEY_PRODUCTIVITY": line1,
    "KEY_TOTAL_HOURS": line2
  };

  Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log("Weather info sent to Pebble successfully!");
    },
    function(e) {
      console.log("Error sending weather info to Pebble!");
    }
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getProductivity();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getProductivity();
  }                     
);
