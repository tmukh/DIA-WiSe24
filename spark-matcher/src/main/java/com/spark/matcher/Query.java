package com.spark.matcher;

import java.io.Serializable;

public class Query implements Serializable {
    private long id;
    private String queryStr;
    private int matchType;
    private int matchDist;
    
    public Query(long id, String queryStr, int matchType, int matchDist) {
        this.id = id;
        this.queryStr = queryStr;
        this.matchType = matchType;
        this.matchDist = matchDist;
    }
    
    public long getId() { return id; }
    public String getQueryStr() { return queryStr; }
    public int getMatchType() { return matchType; }
    public int getMatchDist() { return matchDist; }
}