# onecache

OneCache a Redis protocol based distributed cache middleware, as a replacement of Twenproxy or Codis. Single redis instance is not power enough, so we need groups of redis instances for higher throughput. Unlike twemproxy, OneCache can offer 500,000 or higher QPS for single instance with lower latency. And OneCache can group redis master and slave togather into to one server group for read/write splitting and failover.

For more information, please visit http://www.mysqlsoft.com

For any question, please contact flou(#)onexsoft.com
