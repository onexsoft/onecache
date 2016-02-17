# onecache

OneCache a Redis protocol based distributed cache middleware, as a replacement of Twemproxy or Codis. Single redis instance is not power enough, so we need groups of redis instances for higher throughput. Unlike twemproxy, OneCache can offer 500,000 or higher QPS for single instance with lower latency. And OneCache can group redis master and slave togather into to one server group for read/write splitting and failover.

We have comercial database proxy for MySQL/PostgreSQL database, which can be a good load balancer for MySQL/PostgreSQL groups, or the SQL sharding routers transparently to you applications, or the SQL fireware to prevent your application from SQL injection hurt. For more information, please visit http://www.mysqlsoft.com

For any question, please contact flou(#)onexsoft.com
