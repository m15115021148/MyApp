package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/12/11.
 */

public class BluGroupInvitationModel extends BluBaseModel {
    private GroupInvitation groupInvitation;

    public GroupInvitation getGroupInvitation() {
        return groupInvitation;
    }

    public void setGroupInvitation(GroupInvitation groupInvitation) {
        this.groupInvitation = groupInvitation;
    }

    public static class GroupInvitation {
        private String token;
        private String group_uuid;

        public String getToken() {
            return token;
        }

        public void setToken(String token) {
            this.token = token;
        }

        public String getGroup_uuid() {
            return group_uuid;
        }

        public void setGroup_uuid(String group_uuid) {
            this.group_uuid = group_uuid;
        }
    }
}
