
var socket = false;

function ASSERT(val, msg)
{
    if(!val)
    {
        if(msg)
            alert(msg);
        else
            alert("JavaScript failed");
        throw '';
    }
}

function launchChangeText(tag, text)
{
    var p = tag.parentNode;
    while(p && p.className != 'box')
        p = p.parentNode;

    ASSERT(p, 'JavaScript Error: parentNode with class box not found');

    // find span node child with class state
    var state;
    for(state = p.firstChild; state && state.className != 'state'; state =
            state.nextSibling);

    ASSERT(state, 'JavaScript Error: state span node not found');

    // remove all children
    while(state.firstChild)
        state.removeChild(state.firstChild);
    // add text
    state.appendChild(document.createTextNode(text));
}

function setTagToLaunch(tag)
{
    tag.className = 'launch';
    tag.title = 'run: ' + tag.id;
    tag.onclick = launch;
    launchChangeText(tag, tag.State.stateText);
    ASSERT(tag.State.state == 'dead', "tag.State.state != 'dead'");
}

function turnOffOnClicks(node)
{
    // TODO: May need to be more selective as to what onclicks
    // to disable, with if(node.click && node.class == ??)

    if(node.onclick)
        node.onclick = null;

    // Recursive function turns off onclick for
    // this node and all siblings and all descendents.
    if(node.firstChild)
        turnOffOnClicks(node.firstChild);

    if(node.nextSibling)
        turnOffOnClicks(node.nextSibling);
}

function quit()
{
    var body = document.getElementsByTagName("BODY")[0];
    turnOffOnClicks(body);

    var p = document.createElement("P");
    p.className = 'warning';
    body.className = 'warning';
    body.insertBefore(p, body.firstChild);
    p.appendChild(document.createTextNode("Server at " +
            socket.Url + " has disconnected."));
    socket = null;
}

function kill(t)
{
    if(arguments.length > 0 && t.nodeName)
        var tag = t;
    else
        var tag = this;

    tag.State = { command: 'kill', relPath: tag.id, state: 'running' };
    socket.send(JSON.stringify(tag.State));
}

function setTagToRunning(tag)
{
    tag.className = 'running';
    tag.title = 'kill: ' + tag.id;
    launchChangeText(tag, tag.State.stateText);
    tag.onclick = kill;
}

function main()
{
    // Change 'http' to 'ws' and 'https' to 'wss'
    var socketUrl = location.href.replace(/^http/, 'ws');
    socket = new WebSocket(socketUrl);
    socket.Url = socketUrl;
    socket.onmessage = function(event) {
        //alert('Client received the message ' + event.data);

        var state = JSON.parse(event.data);

        if(state.type && state.type == 'HEADING')
        {
            var n = document.getElementById('heading');
            if(n) n.innerHTML = state.heading;
            return;
        }

        var tag = document.getElementById(state.relPath);

        if(!tag) /* We likely do not have this tag's relPath in this
                    particular page, another client may be using a
                    different page/demo/directory and the server is
                    telling us of an event that we have no interest in.
                    We get the correct state we need if we load different
                    demo runner pages. */
            return;


        tag.State = state;

        if(state.state == 'running')
            setTagToRunning(tag);
        else { // state.state == 'dead'
            ASSERT(state.state == 'dead',
                    'Web Socket received state != "dead"');
            setTagToLaunch(tag);
        }
    };
    socket.onclose = function(event) {
        quit();
    }
}


function quitClick(tag)
{
    // We tell the server to quit and on the sock close
    // event we really quit.
    if(socket)
        socket.send(JSON.stringify({ command: 'quit' }));
    tag.onclick = null;
    tag.className = 'postquit';
}

// We go to a pre-run (post-launch) state until we get a reply from
// the server.
function setTagToPostLaunch(tag)
{
    tag.className = 'postlaunch';
    // The run count is the current count plus 1
    // The server will reply with a new count after this
    // post-launch thingy.
    var runCount = (tag.State) ? tag.State.runCount + 1: 1;

    tag.title = 'Starting (' + runCount + '): ' + tag.id;
    // deactivate the button until we get a reply.
    tag.onclick = null;
    launchChangeText(tag, "Starting (" + runCount + ')');
}

function launch(t)
{
    if(arguments.length > 0 && t.nodeName)
        var tag = t;
    else
        var tag = this;

    var xhttp = new XMLHttpRequest();

    if(!xhttp) {
        alert("XMLHttpRequest Can't connect to server");
        quit();
        return;
    }

    setTagToPostLaunch(tag);

    xhttp.onreadystatechange = function() {
        if (xhttp.readyState == 4 && xhttp.status == 200) {

            //alert("message='" + xhttp.response + "'");

            var tag = this.Tag;
            tag.State = JSON.parse(xhttp.response);

            if(tag.State.state == 'running')
                setTagToRunning(tag);
            else {
                alert('Failed to run: ' + tag.id);
                setTagToLaunch(tag);
            }

            delete this;

        } else {
            // alert("got reply: xhttp.readyState=" + xhttp.readyState + "\nxhttp.status =" + xhttp.status);
        }
    };

    xhttp.Tag = tag;
    // Send the request via HTTP
    xhttp.open('GET', tag.id + '?run', true);
    xhttp.send();

    // The 3 Run/Tag States are: runnable/launch, sent/waiting/postlaunch, running
}

