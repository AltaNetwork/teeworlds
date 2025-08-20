SERVER="./teeworlds_srv"

while true; do
  echo "Starting server"
  $SERVER
  echo "Server crashed! Restarting in 3 seconds"
  sleep 3
done
